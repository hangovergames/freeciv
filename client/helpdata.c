/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/****************************************************************
 This module is for generic handling of help data, independent
 of gui considerations.
*****************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdio.h>
#include <string.h>

/* utility */
#include "astring.h"
#include "bitvector.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "string_vector.h"
#include "support.h"

/* common */
#include "achievements.h"
#include "actions.h"
#include "calendar.h"
#include "city.h"
#include "effects.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "multipliers.h"
#include "requirements.h"
#include "research.h"
#include "road.h"
#include "specialist.h"
#include "tilespec.h"
#include "unit.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "gui_main_g.h" /* client_string */

#include "helpdata.h"

/* helper macro for easy conversion from snprintf and cat_snprintf */
#define CATLSTR(_b, _s, _t) fc_strlcat(_b, _t, _s)

/* This must be in same order as enum in helpdlg_g.h */
static const char * const help_type_names[] = {
  "(Any)", "(Text)", "Units", "Improvements", "Wonders",
  "Techs", "Terrain", "Extras", "Specialists", "Governments",
  "Ruleset", "Tileset", "Nations", "Multipliers", NULL
};

/*define MAX_LAST (MAX(MAX(MAX(A_LAST,B_LAST),U_LAST),terrain_count()))*/

#define SPECLIST_TAG help
#define SPECLIST_TYPE struct help_item
#include "speclist.h"

#define help_list_iterate(helplist, phelp) \
    TYPED_LIST_ITERATE(struct help_item, helplist, phelp)
#define help_list_iterate_end  LIST_ITERATE_END

static const struct help_list_link *help_nodes_iterator;
static struct help_list *help_nodes;
static bool help_nodes_init = FALSE;
/* helpnodes_init is not quite the same as booted in boot_help_texts();
   latter can be 0 even after call, eg if couldn't find helpdata.txt.
*/

/****************************************************************
  Initialize.
*****************************************************************/
void helpdata_init(void)
{
  help_nodes = help_list_new();
}

/****************************************************************
  Clean up.
*****************************************************************/
void helpdata_done(void)
{
  help_list_destroy(help_nodes);
}

/****************************************************************
  Make sure help_nodes is initialised.
  Should call this just about everywhere which uses help_nodes,
  to be careful...  or at least where called by external
  (non-static) functions.
*****************************************************************/
static void check_help_nodes_init(void)
{
  if (!help_nodes_init) {
    help_nodes_init = TRUE;    /* before help_iter_start to avoid recursion! */
    help_iter_start();
  }
}

/****************************************************************
  Free all allocations associated with help_nodes.
*****************************************************************/
void free_help_texts(void)
{
  check_help_nodes_init();
  help_list_iterate(help_nodes, ptmp) {
    free(ptmp->topic);
    free(ptmp->text);
    free(ptmp);
  } help_list_iterate_end;
  help_list_clear(help_nodes);
}

/****************************************************************************
  Returns whether we should show help for this nation.
****************************************************************************/
static bool show_help_for_nation(const struct nation_type *pnation)
{
  return client_nation_is_in_current_set(pnation);
}

/****************************************************************************
  Insert fixed-width table describing veteran system.
  If only one veteran level, inserts 'nolevels' if non-NULL.
  Otherwise, insert 'intro' then a table.
****************************************************************************/
static bool insert_veteran_help(char *outbuf, size_t outlen,
                                const struct veteran_system *veteran,
                                const char *intro, const char *nolevels)
{
  /* game.veteran can be NULL in pregame; if so, keep quiet about
   * veteran levels */
  if (!veteran) {
    return FALSE;
  }

  fc_assert_ret_val(veteran->levels >= 1, FALSE);

  if (veteran->levels == 1) {
    /* Only a single veteran level. Don't bother to name it. */
    if (nolevels) {
      CATLSTR(outbuf, outlen, nolevels);
      return TRUE;
    } else {
      return FALSE;
    }
  } else {
    int i;
    fc_assert_ret_val(veteran->definitions != NULL, FALSE);
    if (intro) {
      CATLSTR(outbuf, outlen, intro);
      CATLSTR(outbuf, outlen, "\n\n");
    }
    /* raise_chance and work_raise_chance don't get to the client, so we
     * can't report them */
    CATLSTR(outbuf, outlen,
            /* TRANS: Header for fixed-width veteran level table.
             * TRANS: Translators cannot change column widths :(
             * TRANS: "Level name" left-justified, other two right-justified */
            _("Veteran level      Power factor   Move bonus\n"));
    CATLSTR(outbuf, outlen,
            /* TRANS: Part of header for veteran level table. */
            _("--------------------------------------------"));
    for (i = 0; i < veteran->levels; i++) {
      const struct veteran_level *level = &veteran->definitions[i];
      const char *name = name_translation_get(&level->name);
      /* Use get_internal_string_length() for correct alignment with
       * multibyte character encodings */
      cat_snprintf(outbuf, outlen,
          "\n%s%*s %4d%% %12s",
          name, MAX(0, 25 - (int)get_internal_string_length(name)), "",
          level->power_fact,
          /* e.g. "-    ", "+ 1/3", "+ 1    ", "+ 2 2/3" */
          move_points_text_full(level->move_bonus, TRUE, "+ ", "-", TRUE));
    }
    return TRUE;
  }
}

/****************************************************************************
  Insert generated text for the helpdata "name".
  Returns TRUE if anything was added.
****************************************************************************/
static bool insert_generated_text(char *outbuf, size_t outlen, const char *name)
{
  if (!game.client.ruleset_init) {
    return FALSE;
  }

  if (0 == strcmp(name, "TerrainAlterations")) {
    int clean_pollution_time = -1, clean_fallout_time = -1;
    bool terrain_independent_extras = FALSE;

    CATLSTR(outbuf, outlen,
            /* TRANS: Header for fixed-width terrain alteration table.
             * TRANS: Translators cannot change column widths :( */
            _("Terrain       Irrigation       Mining           Transform\n"));
    CATLSTR(outbuf, outlen,
            "----------------------------------------------------------------\n");
    terrain_type_iterate(pterrain) {
      if (0 != strlen(terrain_rule_name(pterrain))) {
        char irrigation_time[4], mining_time[4], transform_time[4];
        const char *terrain, *irrigation_result,
                   *mining_result,*transform_result;
        struct universal for_terr = { .kind = VUT_TERRAIN, .value = { .terrain = pterrain }};

        fc_snprintf(irrigation_time, sizeof(irrigation_time),
                    "%d", pterrain->irrigation_time);
        fc_snprintf(mining_time, sizeof(mining_time),
                    "%d", pterrain->mining_time);
        fc_snprintf(transform_time, sizeof(transform_time),
                    "%d", pterrain->transform_time);
        terrain = terrain_name_translation(pterrain);
        irrigation_result = 
          (pterrain->irrigation_result == pterrain
           || pterrain->irrigation_result == T_NONE
           || effect_cumulative_max(EFT_IRRIG_TF_POSSIBLE, &for_terr) <= 0) ? ""
          : terrain_name_translation(pterrain->irrigation_result);
        mining_result =
          (pterrain->mining_result == pterrain
           || pterrain->mining_result == T_NONE
           || effect_cumulative_max(EFT_MINING_TF_POSSIBLE, &for_terr) <= 0) ? ""
          : terrain_name_translation(pterrain->mining_result);
        transform_result =
          (pterrain->transform_result == pterrain
           || pterrain->transform_result == T_NONE
           || effect_cumulative_max(EFT_TRANSFORM_POSSIBLE, &for_terr) <= 0) ? ""
          : terrain_name_translation(pterrain->transform_result);
        /* Use get_internal_string_length() for correct alignment with
         * multibyte character encodings */
        cat_snprintf(outbuf, outlen,
            "%s%*s %3s %s%*s %3s %s%*s %3s %s\n",
            terrain,
            MAX(0, 12 - (int)get_internal_string_length(terrain)), "",
            (pterrain->irrigation_result == T_NONE) ? "-" : irrigation_time,
            irrigation_result,
            MAX(0, 12 - (int)get_internal_string_length(irrigation_result)), "",
            (pterrain->mining_result == T_NONE) ? "-" : mining_time,
            mining_result,
            MAX(0, 12 - (int)get_internal_string_length(mining_result)), "",
            (pterrain->transform_result == T_NONE) ? "-" : transform_time,
            transform_result);

        if (clean_pollution_time != 0) {
          if (clean_pollution_time < 0) {
            clean_pollution_time = pterrain->clean_pollution_time;
          } else {
            if (clean_pollution_time != pterrain->clean_pollution_time) {
              clean_pollution_time = 0; /* give up */
            }
          }
        }
        if (clean_fallout_time != 0) {
          if (clean_fallout_time < 0) {
            clean_fallout_time = pterrain->clean_fallout_time;
          } else {
            if (clean_fallout_time != pterrain->clean_fallout_time) {
              clean_fallout_time = 0; /* give up */
            }
          }
        }
      }
    } terrain_type_iterate_end;

    extra_type_by_cause_iterate(EC_BASE, pextra) {
      if (pextra->buildable && pextra->build_time > 0) {
        terrain_independent_extras = TRUE;
        break;
      }
    } extra_type_by_cause_iterate_end;
    if (!terrain_independent_extras) {
      extra_type_by_cause_iterate(EC_ROAD, pextra) {
        if (pextra->buildable && pextra->build_time > 0) {
          terrain_independent_extras = TRUE;
          break;
        }
      } extra_type_by_cause_iterate_end;
    }

    if (clean_pollution_time > 0 || clean_fallout_time > 0
        || terrain_independent_extras) {
      CATLSTR(outbuf, outlen, "\n");
      CATLSTR(outbuf, outlen,
              _("Time taken for the following activities is independent of "
                "terrain:\n"));
      CATLSTR(outbuf, outlen, "\n");
      CATLSTR(outbuf, outlen,
              /* TRANS: Header for fixed-width terrain alteration table.
               * TRANS: Translators cannot change column widths :( */
              _("Activity            Time\n"));
      CATLSTR(outbuf, outlen,
              "---------------------------");
      if (clean_pollution_time > 0)
	cat_snprintf(outbuf, outlen,
		     _("\nClean pollution    %3d"), clean_pollution_time);
      if (clean_fallout_time > 0)
	cat_snprintf(outbuf, outlen,
		     _("\nClean fallout      %3d"), clean_fallout_time);
      extra_type_by_cause_iterate(EC_ROAD, pextra) {
        if (pextra->buildable && pextra->build_time > 0) {
          const char *rname = extra_name_translation(pextra);

          cat_snprintf(outbuf, outlen,
                       "\n%s%*s %3d",
                       rname,
                       MAX(0, 18 - (int)get_internal_string_length(rname)), "",
                       pextra->build_time);
        }
      } extra_type_by_cause_iterate_end;
      extra_type_by_cause_iterate(EC_BASE, pextra) {
        if (pextra->buildable && pextra->build_time > 0) {
          const char *bname = extra_name_translation(pextra);

          cat_snprintf(outbuf, outlen,
                       "\n%s%*s %3d",
                       bname,
                       MAX(0, 18 - (int)get_internal_string_length(bname)), "",
                       pextra->build_time);
        }
      } extra_type_by_cause_iterate_end;
    }
    return TRUE;
  } else if (0 == strcmp(name, "VeteranLevels")) {
    return insert_veteran_help(outbuf, outlen, game.veteran,
        _("In this ruleset, the following veteran levels are defined:"),
        _("This ruleset has no default veteran levels defined."));
  } else if (0 == strcmp(name, "FreecivVersion")) {
    const char *ver = freeciv_name_version();

    cat_snprintf(outbuf, outlen,
                 /* TRANS: First %s is version string, e.g.,
                  * "Freeciv version 2.3.0-beta1 (beta version)" (translated).
                  * Second %s is client_string, e.g., "gui-gtk-2.0". */
                 _("This is %s, %s client."), ver, client_string);
    insert_client_build_info(outbuf, outlen);

    return TRUE;
  } else if (0 == strcmp(name, "DefaultMetaserver")) {
    cat_snprintf(outbuf, outlen, "  %s", FREECIV_META_URL);

    return TRUE;
  }
  log_error("Unknown directive '$%s' in help", name);
  return FALSE;
}

/****************************************************************
  Append text for the requirement.  Something like

    "Requires knowledge of the technology Communism.\n"

  pplayer may be NULL.  Note that it must be updated everytime
  a new requirement type or range is defined.
*****************************************************************/
static bool insert_requirement(char *buf, size_t bufsz,
                               struct player *pplayer,
                               const struct requirement *preq)
{
  if (preq->quiet) {
    return FALSE;
  }

  switch (preq->source.kind) {
  case VUT_NONE:
    return FALSE;

  case VUT_ADVANCE:
    switch (preq->range) {
    case REQ_RANGE_PLAYER:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires knowledge of the technology %s.\n"),
                     advance_name_translation(preq->source.value.advance));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Prevented by knowledge of the technology %s.\n"),
                     advance_name_translation(preq->source.value.advance));
      }
      return TRUE;
    case REQ_RANGE_TEAM:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires that a player on your team knows the "
                       "technology %s.\n"),
                     advance_name_translation(preq->source.value.advance));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Prevented if any player on your team knows the "
                       "technology %s.\n"),
                     advance_name_translation(preq->source.value.advance));
      }
      return TRUE;
    case REQ_RANGE_ALLIANCE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires that a player allied to you knows the "
                       "technology %s.\n"),
                     advance_name_translation(preq->source.value.advance));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Prevented if any player allied to you knows the "
                       "technology %s.\n"),
                     advance_name_translation(preq->source.value.advance));
      }
      return TRUE;
    case REQ_RANGE_WORLD:
      if (preq->survives) {
        if (preq->present) {
          cat_snprintf(buf, bufsz,
                       _("Requires that someone has discovered the "
                         "technology %s.\n"),
                       advance_name_translation(preq->source.value.advance));
        } else {
          cat_snprintf(buf, bufsz,
                       _("Requires that no-one has yet discovered the "
                        "technology %s.\n"),
                       advance_name_translation(preq->source.value.advance));
        }
      } else {
        if (preq->present) {
          cat_snprintf(buf, bufsz,
                       _("Requires that some player knows the "
                         "technology %s.\n"),
                       advance_name_translation(preq->source.value.advance));
        } else {
          cat_snprintf(buf, bufsz,
                       _("Requires that no player knows the "
                         "technology %s.\n"),
                       advance_name_translation(preq->source.value.advance));
        }
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_TECHFLAG:
    switch (preq->range) {
    case REQ_RANGE_PLAYER:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) tech flag. */
                     _("Requires knowledge of a technology with the "
                       "\"%s\" flag.\n"),
                     tech_flag_id_translated_name(preq->source.value.techflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) tech flag. */
                     _("Prevented by knowledge of any technology with the "
                       "\"%s\" flag.\n"),
                     tech_flag_id_translated_name(preq->source.value.techflag));
      }
      return TRUE;
    case REQ_RANGE_TEAM:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) tech flag. */
                     _("Requires that a player on your team knows "
                       "a technology with the \"%s\" flag.\n"),
                     tech_flag_id_translated_name(preq->source.value.techflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) tech flag. */
                     _("Prevented if any player on your team knows "
                       "any technology with the \"%s\" flag.\n"),
                     tech_flag_id_translated_name(preq->source.value.techflag));
      }
      return TRUE;
    case REQ_RANGE_ALLIANCE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) tech flag. */
                     _("Requires that a player allied to you knows "
                       "a technology with the \"%s\" flag.\n"),
                     tech_flag_id_translated_name(preq->source.value.techflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) tech flag. */
                     _("Prevented if any player allied to you knows "
                       "any technology with the \"%s\" flag.\n"),
                     tech_flag_id_translated_name(preq->source.value.techflag));
      }
      return TRUE;
    case REQ_RANGE_WORLD:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) tech flag. */
                     _("Requires that some player knows a technology "
                       "with the \"%s\" flag.\n"),
                     tech_flag_id_translated_name(preq->source.value.techflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) tech flag. */
                     _("Requires that no player knows any technology with "
                       "the \"%s\" flag.\n"),
                     tech_flag_id_translated_name(preq->source.value.techflag));
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_GOVERNMENT:
    if (preq->range != REQ_RANGE_PLAYER) {
      break;
    }
    if (preq->present) {
      cat_snprintf(buf, bufsz, _("Requires the %s government.\n"),
                   government_name_translation(preq->source.value.govern));
    } else {
      cat_snprintf(buf, bufsz, _("Not available under the %s government.\n"),
                   government_name_translation(preq->source.value.govern));
    }
    return TRUE;

  case VUT_ACHIEVEMENT:
    switch (preq->range) {
    case REQ_RANGE_PLAYER:
      if (preq->present) {
        cat_snprintf(buf, bufsz, _("Requires you to have achieved \"%s\".\n"),
                     achievement_name_translation(preq->source.value.achievement));
      } else {
        cat_snprintf(buf, bufsz, _("Not available once you have achieved "
                                   "\"%s\".\n"),
                     achievement_name_translation(preq->source.value.achievement));
      }
      return TRUE;
    case REQ_RANGE_TEAM:
      if (preq->present) {
        cat_snprintf(buf, bufsz, _("Requires that at least one of your "
                                   "team-mates has achieved \"%s\".\n"),
                     achievement_name_translation(preq->source.value.achievement));
      } else {
        cat_snprintf(buf, bufsz, _("Not available if any of your team-mates "
                                   "has achieved \"%s\".\n"),
                     achievement_name_translation(preq->source.value.achievement));
      }
      return TRUE;
    case REQ_RANGE_ALLIANCE:
      if (preq->present) {
        cat_snprintf(buf, bufsz, _("Requires that at least one of your allies "
                                   "has achieved \"%s\".\n"),
                     achievement_name_translation(preq->source.value.achievement));
      } else {
        cat_snprintf(buf, bufsz, _("Not available if any of your allies has "
                                   "achieved \"%s\".\n"),
                     achievement_name_translation(preq->source.value.achievement));
      }
      return TRUE;
    case REQ_RANGE_WORLD:
      if (preq->present) {
        cat_snprintf(buf, bufsz, _("Requires that at least one player "
                                   "has achieved \"%s\".\n"),
                     achievement_name_translation(preq->source.value.achievement));
      } else {
        cat_snprintf(buf, bufsz, _("Not available if any player has "
                                   "achieved \"%s\".\n"),
                     achievement_name_translation(preq->source.value.achievement));
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_ACTION:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz, _("Applies to the \"%s\" action.\n"),
                     action_name_translation(preq->source.value.action));
      } else {
        cat_snprintf(buf, bufsz, _("Doesn't apply to the \"%s\""
                                   " action.\n"),
                     action_name_translation(preq->source.value.action));
      }
      return TRUE;
    default:
      /* Not supported. */
      break;
    }
    break;

  case VUT_IMPR_GENUS:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz, _("Applies to \"%s\" buildings.\n"),
                     impr_genus_id_translated_name(
                       preq->source.value.impr_genus));
      } else {
        cat_snprintf(buf, bufsz, _("Doesn't apply to \"%s\" buildings.\n"),
                     impr_genus_id_translated_name(
                       preq->source.value.impr_genus));
      }
      return TRUE;
    default:
      /* Not supported. */
      break;
    }
    break;

  case VUT_IMPROVEMENT:
    switch (preq->range) {
    case REQ_RANGE_WORLD:
      if (is_great_wonder(preq->source.value.building)) {
        if (preq->survives) {
          if (preq->present) {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires that %s was built at some point, "
                             "and that it has not yet been rendered "
                             "obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires that %s was built at some point.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          } else {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if %s has ever been built, "
                             "unless it would be obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if %s has ever been built.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          }
        } else {
          /* Non-surviving requirement */
          if (preq->present) {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires %s to be owned by any player "
                             "and not yet obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires %s to be owned by any player.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          } else {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if %s is currently owned by "
                             "any player, unless it is obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if %s is currently owned by "
                             "any player.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          }
        }
        return TRUE;
      }
      /* non-great-wonder world-ranged requirements not supported */
      break;
    case REQ_RANGE_ALLIANCE:
      if (is_wonder(preq->source.value.building)) {
        if (preq->survives) {
          if (preq->present) {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires someone who is currently allied to "
                             "you to have built %s at some point, and for "
                             "it not to have been rendered obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires someone who is currently allied to "
                             "you to have built %s at some point.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          } else {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if someone currently allied to you "
                             "has ever built %s, unless it would be "
                             "obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if someone currently allied to you "
                             "has ever built %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          }
        } else {
          /* Non-surviving requirement */
          if (preq->present) {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires someone allied to you to own %s, "
                             "and for it not to have been rendered "
                             "obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires someone allied to you to own %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          } else {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if someone allied to you owns %s, "
                             "unless it is obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if someone allied to you owns %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          }
        }
        return TRUE;
      }
      /* non-wonder alliance-ranged requirements not supported */
      break;
    case REQ_RANGE_TEAM:
      if (is_wonder(preq->source.value.building)) {
        if (preq->survives) {
          if (preq->present) {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires someone on your team to have "
                             "built %s at some point, and for it not "
                             "to have been rendered obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires someone on your team to have "
                             "built %s at some point.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          } else {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if someone on your team has ever "
                             "built %s, unless it would be obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if someone on your team has ever "
                             "built %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          }
        } else {
          /* Non-surviving requirement */
          if (preq->present) {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires someone on your team to own %s, "
                             "and for it not to have been rendered "
                             "obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires someone on your team to own %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          } else {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if someone on your team owns %s, "
                             "unless it is obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if someone on your team owns %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          }
        }
        return TRUE;
      }
      /* non-wonder team-ranged requirements not supported */
      break;
    case REQ_RANGE_PLAYER:
      if (is_wonder(preq->source.value.building)) {
        if (preq->survives) {
          if (preq->present) {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires you to have built %s at some point, "
                             "and for it not to have been rendered "
                             "obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires you to have built %s at some point.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          } else {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if you have ever built %s, "
                             "unless it would be obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if you have ever built %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          }
        } else {
          /* Non-surviving requirement */
          if (preq->present) {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires you to own %s, which must not "
                             "be obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Requires you to own %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          } else {
            if (can_improvement_go_obsolete(preq->source.value.building)) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if you own %s, unless it is "
                             "obsolete.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            } else {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is a wonder */
                           _("Prevented if you own %s.\n"),
                           improvement_name_translation
                           (preq->source.value.building));
            }
          }
        }
        return TRUE;
      }
      /* non-wonder player-ranged requirements not supported */
      break;
    case REQ_RANGE_CONTINENT:
      if (is_wonder(preq->source.value.building)) {
        if (preq->present) {
          if (can_improvement_go_obsolete(preq->source.value.building)) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is a wonder */
                         _("Requires %s in one of your cities on the same "
                           "continent, and not yet obsolete.\n"),
                         improvement_name_translation
                         (preq->source.value.building));
          } else {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is a wonder */
                         _("Requires %s in one of your cities on the same "
                           "continent.\n"),
                         improvement_name_translation
                         (preq->source.value.building));
          }
        } else {
          if (can_improvement_go_obsolete(preq->source.value.building)) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is a wonder */
                         _("Prevented if %s is in one of your cities on the "
                           "same continent, unless it is obsolete.\n"),
                         improvement_name_translation
                         (preq->source.value.building));
          } else {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is a wonder */
                         _("Prevented if %s is in one of your cities on the "
                           "same continent.\n"),
                         improvement_name_translation
                         (preq->source.value.building));
          }
        }
        return TRUE;
      }
      /* surviving or non-wonder continent-ranged requirements not supported */
      break;
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        if (can_improvement_go_obsolete(preq->source.value.building)) {
          /* Should only apply to wonders */
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is a building or wonder */
                       _("Requires %s in the city or a trade partner "
                         "(and not yet obsolete).\n"),
                       improvement_name_translation
                       (preq->source.value.building));
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is a building or wonder */
                       _("Requires %s in the city or a trade partner.\n"),
                       improvement_name_translation
                       (preq->source.value.building));
        }
      } else {
        if (can_improvement_go_obsolete(preq->source.value.building)) {
          /* Should only apply to wonders */
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is a building or wonder */
                       _("Prevented by %s in the city or a trade partner "
                         "(unless it is obsolete).\n"),
                       improvement_name_translation
                       (preq->source.value.building));
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is a building or wonder */
                       _("Prevented by %s in the city or a trade partner.\n"),
                       improvement_name_translation
                       (preq->source.value.building));
        }
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        if (can_improvement_go_obsolete(preq->source.value.building)) {
          /* Should only apply to wonders */
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is a building or wonder */
                       _("Requires %s in the city (and not yet obsolete).\n"),
                       improvement_name_translation
                       (preq->source.value.building));
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is a building or wonder */
                       _("Requires %s in the city.\n"),
                       improvement_name_translation
                       (preq->source.value.building));
        }
      } else {
        if (can_improvement_go_obsolete(preq->source.value.building)) {
          /* Should only apply to wonders */
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is a building or wonder */
                       _("Prevented by %s in the city (unless it is "
                         "obsolete).\n"),
                       improvement_name_translation
                       (preq->source.value.building));
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is a building or wonder */
                       _("Prevented by %s in the city.\n"),
                       improvement_name_translation
                       (preq->source.value.building));
        }
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Only applies to \"%s\" buildings.\n"),
                     improvement_name_translation
                     (preq->source.value.building));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Does not apply to \"%s\" buildings.\n"),
                     improvement_name_translation
                     (preq->source.value.building));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_EXTRA:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Requires %s on the tile.\n"),
                     extra_name_translation(preq->source.value.extra));
      } else {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Prevented by %s on the tile.\n"),
                     extra_name_translation(preq->source.value.extra));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Requires %s on the tile or a cardinally "
                        "adjacent tile.\n"),
                     extra_name_translation(preq->source.value.extra));
        } else {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Prevented by %s on the tile or any cardinally "
                        "adjacent tile.\n"),
                     extra_name_translation(preq->source.value.extra));
        }
      return TRUE;
    case REQ_RANGE_ADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Requires %s on the tile or an adjacent "
                        "tile.\n"),
                     extra_name_translation(preq->source.value.extra));
      } else {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Prevented by %s on the tile or any adjacent "
                        "tile.\n"),
                     extra_name_translation(preq->source.value.extra));
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Requires %s on a tile within the city "
                        "radius.\n"),
                     extra_name_translation(preq->source.value.extra));
      } else {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Prevented by %s on any tile within the city "
                        "radius.\n"),
                     extra_name_translation(preq->source.value.extra));
      }
      return TRUE;
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Requires %s on a tile within the city "
                        "radius, or the city radius of a trade partner.\n"),
                     extra_name_translation(preq->source.value.extra));
      } else {
        cat_snprintf(buf, bufsz,
                     Q_("?extra:Prevented by %s on any tile within the city "
                        "radius or the city radius of a trade partner.\n"),
                     extra_name_translation(preq->source.value.extra));
      }
      return TRUE;
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_GOOD:
    switch (preq->range) {
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz, Q_("?good:Requires import of %s .\n"),
                     goods_name_translation(preq->source.value.good));
      } else {
        cat_snprintf(buf, bufsz, Q_("?goods:Prevented by import of %s.\n"),
                     goods_name_translation(preq->source.value.good));
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_TERRAIN:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz, Q_("?terrain:Requires %s on the tile.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      } else {
        cat_snprintf(buf, bufsz, Q_("?terrain:Prevented by %s on the tile.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?terrain:Requires %s on the tile or a cardinally "
                        "adjacent tile.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      } else {
        cat_snprintf(buf, bufsz,
                     Q_("?terrain:Prevented by %s on the tile or any "
                        "cardinally adjacent tile.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      }
      return TRUE;
    case REQ_RANGE_ADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?terrain:Requires %s on the tile or an adjacent "
                        "tile.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      } else {
        cat_snprintf(buf, bufsz,
                     Q_("?terrain:Prevented by %s on the tile or any "
                        "adjacent tile.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?terrain:Requires %s on a tile within the city "
                        "radius.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      } else {
        cat_snprintf(buf, bufsz,
                     Q_("?terrain:Prevented by %s on any tile within the city "
                        "radius.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      }
      return TRUE;
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     Q_("?terrain:Requires %s on a tile within the city "
                        "radius, or the city radius of a trade partner.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      } else {
        cat_snprintf(buf, bufsz,
                     Q_("?terrain:Prevented by %s on any tile within the city "
                        "radius or the city radius of a trade partner.\n"),
                     terrain_name_translation(preq->source.value.terrain));
      }
      return TRUE;
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_NATION:
    switch (preq->range) {
    case REQ_RANGE_PLAYER:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "... playing as the Swedes." */
                     _("Requires that you are playing as the %s.\n"),
                     nation_plural_translation(preq->source.value.nation));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "... playing as the Turks." */
                     _("Requires that you are not playing as the %s.\n"),
                     nation_plural_translation(preq->source.value.nation));
      }
      return TRUE;
    case REQ_RANGE_TEAM:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "... same team as the Indonesians." */
                     _("Requires that you are on the same team as "
                       "the %s.\n"),
                     nation_plural_translation(preq->source.value.nation));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "... same team as the Greeks." */
                     _("Requires that you are not on the same team as "
                       "the %s.\n"),
                     nation_plural_translation(preq->source.value.nation));
      }
      return TRUE;
    case REQ_RANGE_ALLIANCE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "... allied with the Koreans." */
                     _("Requires that you are allied with the %s.\n"),
                     nation_plural_translation(preq->source.value.nation));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "... allied with the Danes." */
                     _("Requires that you are not allied with the %s.\n"),
                     nation_plural_translation(preq->source.value.nation));
      }
      return TRUE;
    case REQ_RANGE_WORLD:
      if (preq->survives) {
        if (preq->present) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: "Requires the Apaches to have ..." */
                       _("Requires the %s to have been in the game.\n"),
                       nation_plural_translation(preq->source.value.nation));
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: "Requires the Celts never to have ..." */
                       _("Requires the %s never to have been in the "
                         "game.\n"),
                       nation_plural_translation(preq->source.value.nation));
        }
      } else {
        if (preq->present) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: "Requires the Belgians in the game." */
                       _("Requires the %s in the game.\n"),
                       nation_plural_translation(preq->source.value.nation));
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: "Requires that the Russians are not ... */
                       _("Requires that the %s are not in the game.\n"),
                       nation_plural_translation(preq->source.value.nation));
        }
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_NATIONGROUP:
    switch (preq->range) {
    case REQ_RANGE_PLAYER:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: nation group: "... playing African nation." */
                     _("Requires that you are playing %s nation.\n"),
                     nation_group_name_translation(preq->source.value.nationgroup));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: nation group: "... playing Imaginary nation." */
                     _("Prevented if you are playing %s nation.\n"),
                     nation_group_name_translation(preq->source.value.nationgroup));
      }
      return TRUE;
    case REQ_RANGE_TEAM:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: nation group: "Requires Medieval nation ..." */
                     _("Requires %s nation on your team.\n"),
                     nation_group_name_translation(preq->source.value.nationgroup));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: nation group: "Prevented by Medieval nation ..." */
                     _("Prevented by %s nation on your team.\n"),
                     nation_group_name_translation(preq->source.value.nationgroup));
      }
      return TRUE;
    case REQ_RANGE_ALLIANCE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: nation group: "Requires Modern nation ..." */
                     _("Requires %s nation in alliance with you.\n"),
                     nation_group_name_translation(preq->source.value.nationgroup));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: nation group: "Prevented by Modern nation ..." */
                     _("Prevented if %s nation is in alliance with you.\n"),
                     nation_group_name_translation(preq->source.value.nationgroup));
      }
      return TRUE;
    case REQ_RANGE_WORLD:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: nation group: "Requires Asian nation ..." */
                     _("Requires %s nation in the game.\n"),
                     nation_group_name_translation(preq->source.value.nationgroup));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: nation group: "Prevented by Asian nation ..." */
                     _("Prevented by %s nation in the game.\n"),
                     nation_group_name_translation(preq->source.value.nationgroup));
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_STYLE:
    if (preq->range != REQ_RANGE_PLAYER) {
      break;
    }
    if (preq->present) {
      cat_snprintf(buf, bufsz,
                   /* TRANS: "Requires that you are playing Asian style 
                    * nation." */
                   _("Requires that you are playing %s style nation.\n"),
                   style_name_translation(preq->source.value.style));
    } else {
      cat_snprintf(buf, bufsz,
                   /* TRANS: "Requires that you are not playing Classical
                    * style nation." */
                   _("Requires that you are not playing %s style nation.\n"),
                   style_name_translation(preq->source.value.style));
    }
    return TRUE;

  case VUT_NATIONALITY:
    switch (preq->range) {
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "Requires at least one Barbarian citizen ..." */
                     _("Requires at least one %s citizen in the city or a "
                       "trade partner.\n"),
                     nation_adjective_translation(preq->source.value.nationality));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "... no Pirate citizens ..." */
                     _("Requires that there are no %s citizens in "
                       "the city or any trade partners.\n"),
                     nation_adjective_translation(preq->source.value.nationality));
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "Requires at least one Barbarian citizen ..." */
                     _("Requires at least one %s citizen in the city.\n"),
                     nation_adjective_translation(preq->source.value.nationality));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: "... no Pirate citizens ..." */
                     _("Requires that there are no %s citizens in "
                       "the city.\n"),
                     nation_adjective_translation(preq->source.value.nationality));
      }
      return TRUE;
    case REQ_RANGE_WORLD:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_DIPLREL:
    switch (preq->range) {
    case REQ_RANGE_PLAYER:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires that your diplomatic relationship to at "
                       "least one other living player is %s.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Requires that no diplomatic relationship you have "
                       "to any living player is %s.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      }
      return TRUE;
    case REQ_RANGE_TEAM:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires that somebody on your team has %s "
                       "diplomatic relationship to another living player.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Requires that nobody on your team has %s "
                       "diplomatic relationship to another living player.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      }
      return TRUE;
    case REQ_RANGE_ALLIANCE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires that somebody in your alliance has %s "
                       "diplomatic relationship to another living player.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Requires that nobody in your alliance has %s "
                       "diplomatic relationship to another living player.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      }
      return TRUE;
    case REQ_RANGE_WORLD:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires the diplomatic relationship %s between two "
                       "living players.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Requires the absence of the diplomatic "
                       "relationship %s between any living players.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires that your diplomatic relationship to the "
                       "other player is %s.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Requires that your diplomatic relationship to the "
                       "other player isn't %s.\n"),
                     diplrel_name_translation(preq->source.value.diplrel));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_UTYPE:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        /* TRANS: %s is a single kind of unit (e.g., "Settlers"). */
        cat_snprintf(buf, bufsz, Q_("?unit:Requires %s.\n"),
                     utype_name_translation(preq->source.value.utype));
      } else {
        /* TRANS: %s is a single kind of unit (e.g., "Settlers"). */
        cat_snprintf(buf, bufsz, Q_("?unit:Does not apply to %s.\n"),
                     utype_name_translation(preq->source.value.utype));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_UTFLAG:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      {
        struct astring astr = ASTRING_INIT;

         /* Unit type flags mean nothing to users. Explicitly list the unit
         * types with those flags. */
        if (role_units_translations(&astr, preq->source.value.unitflag,
                                    TRUE)) {
          if (preq->present) {
            /* TRANS: %s is a list of unit types separated by "or". */
            cat_snprintf(buf, bufsz, Q_("?ulist:Requires %s.\n"),
                         astr_str(&astr));
          } else {
            /* TRANS: %s is a list of unit types separated by "or". */
            cat_snprintf(buf, bufsz, Q_("?ulist:Does not apply to %s.\n"),
                         astr_str(&astr));
          }
          astr_free(&astr);
          return TRUE;
        }
      }
      break;
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_UCLASS:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        /* TRANS: %s is a single unit class (e.g., "Air"). */
        cat_snprintf(buf, bufsz, Q_("?uclass:Requires %s units.\n"),
                     uclass_name_translation(preq->source.value.uclass));
      } else {
        /* TRANS: %s is a single unit class (e.g., "Air"). */
        cat_snprintf(buf, bufsz, Q_("?uclass:Does not apply to %s units.\n"),
                     uclass_name_translation(preq->source.value.uclass));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_UCFLAG:
    {
      const char *classes[uclass_count()];
      int i = 0;
      bool done = FALSE;
      struct astring list = ASTRING_INIT;

      unit_class_iterate(uclass) {
        if (uclass_has_flag(uclass, preq->source.value.unitclassflag)) {
          classes[i++] = uclass_name_translation(uclass);
        }
      } unit_class_iterate_end;
      astr_build_or_list(&list, classes, i);

      switch (preq->range) {
      case REQ_RANGE_LOCAL:
        if (preq->present) {
          /* TRANS: %s is a list of unit classes separated by "or". */
          cat_snprintf(buf, bufsz, Q_("?uclasslist:Requires %s units.\n"),
                       astr_str(&list));
        } else { 
          /* TRANS: %s is a list of unit classes separated by "or". */
          cat_snprintf(buf, bufsz, Q_("?uclasslist:Does not apply to "
                                      "%s units.\n"),
                       astr_str(&list));
        }
        done = TRUE;
        break;
      case REQ_RANGE_CADJACENT:
      case REQ_RANGE_ADJACENT:
      case REQ_RANGE_CITY:
      case REQ_RANGE_TRADEROUTE:
      case REQ_RANGE_CONTINENT:
      case REQ_RANGE_PLAYER:
      case REQ_RANGE_TEAM:
      case REQ_RANGE_ALLIANCE:
      case REQ_RANGE_WORLD:
      case REQ_RANGE_COUNT:
        /* Not supported. */
        break;
      }
      astr_free(&list);
      if (done) {
        return TRUE;
      }
    }
    break;

  case VUT_UNITSTATE:
    {
      switch (preq->range) {
      case REQ_RANGE_LOCAL:
        switch (preq->source.value.unit_state) {
        case USP_TRANSPORTED:
          if (preq->present) {
            cat_snprintf(buf, bufsz,
                         _("Requires that the unit is transported.\n"));
          } else {
            cat_snprintf(buf, bufsz,
                         _("Requires that the unit isn't transported.\n"));
          }
          return TRUE;
        case USP_LIVABLE_TILE:
          if (preq->present) {
            cat_snprintf(buf, bufsz,
                         _("Requires that the unit is on livable tile.\n"));
          } else {
            cat_snprintf(buf, bufsz,
                         _("Requires that the unit isn't on livable tile.\n"));
          }
          return TRUE;
        case USP_DOMESTIC_TILE:
          if (preq->present) {
            cat_snprintf(buf, bufsz,
                         _("Requires that the unit is on a domestic "
                           "tile.\n"));
          } else {
            cat_snprintf(buf, bufsz,
                         _("Requires that the unit isn't on a domestic "
                           "tile.\n"));
          }
          return TRUE;
        case USP_COUNT:
          fc_assert_msg(preq->source.value.unit_state != USP_COUNT,
                        "Invalid unit state property.");
        }
        break;
      case REQ_RANGE_CADJACENT:
      case REQ_RANGE_ADJACENT:
      case REQ_RANGE_CITY:
      case REQ_RANGE_TRADEROUTE:
      case REQ_RANGE_CONTINENT:
      case REQ_RANGE_PLAYER:
      case REQ_RANGE_TEAM:
      case REQ_RANGE_ALLIANCE:
      case REQ_RANGE_WORLD:
      case REQ_RANGE_COUNT:
        /* Not supported. */
        break;
      }
    }
    break;

  case VUT_MINMOVES:
    {
      switch (preq->range) {
      case REQ_RANGE_LOCAL:
        if (preq->present) {
          cat_snprintf(buf, bufsz,
                       /* %s is numeric move points; it may have a
                        * fractional part ("1 1/3 MP"). */
                       _("Requires that the unit has at least %s MP left.\n"),
                       move_points_text(preq->source.value.minmoves, TRUE));
        } else {
          cat_snprintf(buf, bufsz,
                       /* %s is numeric move points; it may have a
                        * fractional part ("1 1/3 MP"). */
                       _("Requires that the unit has less than %s MP left.\n"),
                       move_points_text(preq->source.value.minmoves, TRUE));
        }
        return TRUE;
      case REQ_RANGE_CADJACENT:
      case REQ_RANGE_ADJACENT:
      case REQ_RANGE_CITY:
      case REQ_RANGE_TRADEROUTE:
      case REQ_RANGE_CONTINENT:
      case REQ_RANGE_PLAYER:
      case REQ_RANGE_TEAM:
      case REQ_RANGE_ALLIANCE:
      case REQ_RANGE_WORLD:
      case REQ_RANGE_COUNT:
        /* Not supported. */
        break;
      }
    }
    break;

  case VUT_MINVETERAN:
    if (preq->range != REQ_RANGE_LOCAL) {
      break;
    }
    /* FIXME: this would be better with veteran level names, but that's
     * potentially unit type dependent. */
    if (preq->present) {
      cat_snprintf(buf, bufsz,
                   PL_("Requires a unit with at least %d veteran level.\n",
                       "Requires a unit with at least %d veteran levels.\n",
                       preq->source.value.minveteran),
                   preq->source.value.minveteran);
    } else {
      cat_snprintf(buf, bufsz,
                   PL_("Requires a unit with fewer than %d veteran level.\n",
                       "Requires a unit with fewer than %d veteran levels.\n",
                       preq->source.value.minveteran),
                   preq->source.value.minveteran);
    }
    return TRUE;

    case VUT_MINHP:
      if (preq->range != REQ_RANGE_LOCAL) {
        break;
      }

      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires a unit with at least %d hit point left.\n",
                         "Requires a unit with at least %d hit points left.\n",
                         preq->source.value.min_hit_points),
                     preq->source.value.min_hit_points);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Requires a unit with fewer than %d hit point "
                         "left.\n",
                         "Requires a unit with fewer than %d hit points "
                         "left.\n",
                         preq->source.value.min_hit_points),
                     preq->source.value.min_hit_points);
      }
      return TRUE;

  case VUT_OTYPE:
    if (preq->range != REQ_RANGE_LOCAL) {
      break;
    }
    if (preq->present) {
      /* TRANS: "Applies only to Food." */
      cat_snprintf(buf, bufsz, Q_("?output:Applies only to %s.\n"),
                   get_output_name(preq->source.value.outputtype));
    } else {
      /* TRANS: "Does not apply to Food." */
      cat_snprintf(buf, bufsz, Q_("?output:Does not apply to %s.\n"),
                   get_output_name(preq->source.value.outputtype));
    }
    return TRUE;

  case VUT_SPECIALIST:
    if (preq->range != REQ_RANGE_LOCAL) {
      break;
    }
    if (preq->present) {
      /* TRANS: "Applies only to Scientists." */
      cat_snprintf(buf, bufsz, Q_("?specialist:Applies only to %s.\n"),
                   specialist_plural_translation(preq->source.value.specialist));
    } else {
      /* TRANS: "Does not apply to Scientists." */
      cat_snprintf(buf, bufsz, Q_("?specialist:Does not apply to %s.\n"),
                   specialist_plural_translation(preq->source.value.specialist));
    }
    return TRUE;

  case VUT_MINSIZE:
    switch (preq->range) {
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires a minimum city size of %d for this "
                         "city or a trade partner.\n",
                         "Requires a minimum city size of %d for this "
                         "city or a trade partner.\n",
                         preq->source.value.minsize),
                     preq->source.value.minsize);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Requires the city size to be less than %d "
                         "for this city and all trade partners.\n",
                         "Requires the city size to be less than %d "
                         "for this city and all trade partners.\n",
                         preq->source.value.minsize),
                     preq->source.value.minsize);
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires a minimum city size of %d.\n",
                         "Requires a minimum city size of %d.\n",
                         preq->source.value.minsize),
                     preq->source.value.minsize);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Requires the city size to be less than %d.\n",
                         "Requires the city size to be less than %d.\n",
                         preq->source.value.minsize),
                   preq->source.value.minsize);
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }

  case VUT_MINCULTURE:
    switch (preq->range) {
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires a minimum culture of %d in the city.\n",
                         "Requires a minimum culture of %d in the city.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Requires the culture in the city to be less "
                         "than %d.\n",
                         "Requires the culture in the city to be less "
                         "than %d.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      }
      return TRUE;
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires a minimum culture of %d in this city or "
                         "a trade partner.\n",
                         "Requires a minimum culture of %d in this city or "
                         "a trade partner.\n",
                         preq->source.value.minculture),
                      preq->source.value.minculture);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Requires the culture in this city and all trade "
                         "partners to be less than %d.\n",
                         "Requires the culture in this city and all trade "
                         "partners to be less than %d.\n",
                         preq->source.value.minculture),
                      preq->source.value.minculture);
      }
      return TRUE;
    case REQ_RANGE_PLAYER:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires your nation to have culture "
                         "of at least %d.\n",
                         "Requires your nation to have culture "
                         "of at least %d.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Prevented if your nation has culture of "
                         "%d or more.\n",
                         "Prevented if your nation has culture of "
                         "%d or more.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      }
      return TRUE;
    case REQ_RANGE_TEAM:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires someone on your team to have culture of "
                         "at least %d.\n",
                         "Requires someone on your team to have culture of "
                         "at least %d.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Prevented if anyone on your team has culture of "
                         "%d or more.\n",
                         "Prevented if anyone on your team has culture of "
                         "%d or more.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      }
      return TRUE;
    case REQ_RANGE_ALLIANCE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires someone in your current alliance to "
                         "have culture of at least %d.\n",
                         "Requires someone in your current alliance to "
                         "have culture of at least %d.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Prevented if anyone in your current alliance has "
                         "culture of %d or more.\n",
                         "Prevented if anyone in your current alliance has "
                         "culture of %d or more.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      }
      return TRUE;
    case REQ_RANGE_WORLD:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("Requires that some player has culture of at "
                         "least %d.\n",
                         "Requires that some player has culture of at "
                         "least %d.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("Requires that no player has culture of %d "
                         "or more.\n",
                         "Requires that no player has culture of %d "
                         "or more.\n",
                         preq->source.value.minculture),
                     preq->source.value.minculture);
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_COUNT:
      break;
    }
    break;

  case VUT_MAXTILEUNITS:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("At most %d unit may be present on the tile.\n",
                         "At most %d units may be present on the tile.\n",
                         preq->source.value.max_tile_units),
                     preq->source.value.max_tile_units);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("There must be more than %d unit present on "
                         "the tile.\n",
                         "There must be more than %d units present on "
                         "the tile.\n",
                         preq->source.value.max_tile_units),
                     preq->source.value.max_tile_units);
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("The tile or at least one cardinally adjacent tile "
                         "must have %d unit or fewer.\n",
                         "The tile or at least one cardinally adjacent tile "
                         "must have %d units or fewer.\n",
                         preq->source.value.max_tile_units),
                     preq->source.value.max_tile_units);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("The tile and all cardinally adjacent tiles must "
                         "have more than %d unit each.\n",
                         "The tile and all cardinally adjacent tiles must "
                         "have more than %d units each.\n",
                         preq->source.value.max_tile_units),
                     preq->source.value.max_tile_units);
      }
      return TRUE;
    case REQ_RANGE_ADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     PL_("The tile or at least one adjacent tile must have "
                         "%d unit or fewer.\n",
                         "The tile or at least one adjacent tile must have "
                         "%d units or fewer.\n",
                         preq->source.value.max_tile_units),
                     preq->source.value.max_tile_units);
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("The tile and all adjacent tiles must have more "
                         "than %d unit each.\n",
                         "The tile and all adjacent tiles must have more "
                         "than %d units each.\n",
                         preq->source.value.max_tile_units),
                     preq->source.value.max_tile_units);
      }
      return TRUE;
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }

  case VUT_AI_LEVEL:
    if (preq->range != REQ_RANGE_PLAYER) {
      break;
    }
    if (preq->present) {
      cat_snprintf(buf, bufsz,
                   /* TRANS: AI level (e.g., "Handicapped") */
                   _("Applies to %s AI players.\n"),
                   ai_level_translated_name(preq->source.value.ai_level));
    } else {
      cat_snprintf(buf, bufsz,
                   /* TRANS: AI level (e.g., "Cheating") */
                   _("Does not apply to %s AI players.\n"),
                   ai_level_translated_name(preq->source.value.ai_level));
    }
    return TRUE;

  case VUT_TERRAINCLASS:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Requires %s terrain on the tile.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Prevented by %s terrain on the tile.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Requires %s terrain on the tile or a "
                        "cardinally adjacent tile.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Prevented by %s terrain on the tile or "
                        "any cardinally adjacent tile.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      }
      return TRUE;
    case REQ_RANGE_ADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Requires %s terrain on the tile or an "
                        "adjacent tile.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Prevented by %s terrain on the tile or "
                        "any adjacent tile.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Requires %s terrain on a tile within "
                        "the city radius.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Prevented by %s terrain on any tile "
                        "within the city radius.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      }
      return TRUE;
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Requires %s terrain on a tile within "
                        "the city radius or the city radius of a trade "
                        "partner.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a terrain class */
                     Q_("?terrainclass:Prevented by %s terrain on any tile "
                        "within the city radius or the city radius of a trade "
                        "partner.\n"),
                     terrain_class_name_translation
                     (preq->source.value.terrainclass));
      }
      return TRUE;
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_TERRFLAG:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Requires terrain with the \"%s\" flag on the tile.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Prevented by terrain with the \"%s\" flag on the "
                       "tile.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Requires terrain with the \"%s\" flag on the "
                       "tile or a cardinally adjacent tile.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Prevented by terrain with the \"%s\" flag on "
                       "the tile or any cardinally adjacent tile.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      }
      return TRUE;
    case REQ_RANGE_ADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Requires terrain with the \"%s\" flag on the "
                       "tile or an adjacent tile.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Prevented by terrain with the \"%s\" flag on "
                       "the tile or any adjacent tile.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Requires terrain with the \"%s\" flag on a tile "
                       "within the city radius.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Prevented by terrain with the \"%s\" flag on any tile "
                       "within the city radius.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      }
      return TRUE;
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Requires terrain with the \"%s\" flag on a tile "
                       "within the city radius or the city radius of "
                       "a trade partner.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) terrain flag. */
                     _("Prevented by terrain with the \"%s\" flag on any tile "
                       "within the city radius or the city radius of "
                       "a trade partner.\n"),
                     terrain_flag_id_translated_name(preq->source.value.terrainflag));
      }
      return TRUE;
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_BASEFLAG:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Requires a base with the \"%s\" flag on the tile.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Prevented by a base with the \"%s\" flag on the "
                       "tile.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Requires a base with the \"%s\" flag on the "
                       "tile or a cardinally adjacent tile.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Prevented by a base with the \"%s\" flag on "
                       "the tile or any cardinally adjacent tile.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      }
      return TRUE;
    case REQ_RANGE_ADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Requires a base with the \"%s\" flag on the "
                       "tile or an adjacent tile.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Prevented by a base with the \"%s\" flag on "
                       "the tile or any adjacent tile.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Requires a base with the \"%s\" flag on a tile "
                       "within the city radius.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Prevented by a base with the \"%s\" flag on any tile "
                       "within the city radius.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      }
      return TRUE;
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Requires a base with the \"%s\" flag on a tile "
                       "within the city radius or the city radius of a "
                       "trade partner.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) base flag. */
                     _("Prevented by a base with the \"%s\" flag on any tile "
                       "within the city radius or the city radius of a "
                       "trade partner.\n"),
                     base_flag_id_translated_name(preq->source.value.baseflag));
      }
      return TRUE;
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_ROADFLAG:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Requires a road with the \"%s\" flag on the tile.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Prevented by a road with the \"%s\" flag on the "
                       "tile.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Requires a road with the \"%s\" flag on the "
                       "tile or a cardinally adjacent tile.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Prevented by a road with the \"%s\" flag on "
                       "the tile or any cardinally adjacent tile.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      }
      return TRUE;
    case REQ_RANGE_ADJACENT:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Requires a road with the \"%s\" flag on the "
                       "tile or an adjacent tile.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Prevented by a road with the \"%s\" flag on "
                       "the tile or any adjacent tile.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      }
      return TRUE;
    case REQ_RANGE_CITY:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Requires a road with the \"%s\" flag on a tile "
                       "within the city radius.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Prevented by a road with the \"%s\" flag on any tile "
                       "within the city radius.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      }
      return TRUE;
    case REQ_RANGE_TRADEROUTE:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Requires a road with the \"%s\" flag on a tile "
                       "within the city radius or the city radius of a "
                       "trade partner.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a (translatable) road flag. */
                     _("Prevented by a road with the \"%s\" flag on any tile "
                       "within the city radius or the city radius of a "
                       "trade partner.\n"),
                     road_flag_id_translated_name(preq->source.value.roadflag));
      }
      return TRUE;
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_MINYEAR:
    if (preq->range != REQ_RANGE_WORLD) {
      break;
    }
    if (preq->present) {
      cat_snprintf(buf, bufsz,
                   _("Requires the game to have reached the year %s.\n"),
                   textyear(preq->source.value.minyear));
    } else {
      cat_snprintf(buf, bufsz,
                   _("Requires that the game has not yet reached the "
                     "year %s.\n"),
                   textyear(preq->source.value.minyear));
    }
    return TRUE;

  case VUT_TOPO:
    if (preq->range != REQ_RANGE_WORLD) {
      break;
    }
    if (preq->present) {
      cat_snprintf(buf, bufsz,
                   _("Requires %s map.\n"),
                   _(topo_flag_name(preq->source.value.topo_property)));
    } else {
      cat_snprintf(buf, bufsz,
                   _("Prevented on %s map.\n"),
                   _(topo_flag_name(preq->source.value.topo_property)));
    }
    return TRUE;

  case VUT_AGE:
    if (preq->present) {
      cat_snprintf(buf, bufsz,
                   _("Requires age of %d turns.\n"),
                   preq->source.value.age);
    } else {
      cat_snprintf(buf, bufsz,
                   _("Prevented if age is over %d turns.\n"),
                   preq->source.value.age);
    }
    return TRUE;

  case VUT_MINTECHS:
    switch (preq->range) {
    case REQ_RANGE_WORLD:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires %d techs to be known in the world.\n"),
                     preq->source.value.min_techs);
      } else {
        cat_snprintf(buf, bufsz,
                     _("Prevented when %d techs are known in the world.\n"),
                     preq->source.value.min_techs);
      }
      return TRUE;
    case REQ_RANGE_PLAYER:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires player to knoe %d techs.\n"),
                     preq->source.value.min_techs);
      } else {
        cat_snprintf(buf, bufsz,
                     _("Prevented when player knows %d techs.\n"),
                     preq->source.value.min_techs);
      }
      return TRUE;
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_TERRAINALTER:
    switch (preq->range) {
    case REQ_RANGE_LOCAL:
      if (preq->present) {
        cat_snprintf(buf, bufsz,
                     _("Requires terrain on which alteration %s is "
                       "possible.\n"),
                     Q_(terrain_alteration_name(preq->source.value.terrainalter)));
      } else {
        cat_snprintf(buf, bufsz,
                     _("Prevented by terrain on which alteration %s "
                       "can be made.\n"),
                     Q_(terrain_alteration_name(preq->source.value.terrainalter)));
      }
      return TRUE;
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
    case REQ_RANGE_CITY:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_PLAYER:
    case REQ_RANGE_TEAM:
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_WORLD:
    case REQ_RANGE_COUNT:
      /* Not supported. */
      break;
    }
    break;

  case VUT_CITYTILE:
    if (preq->source.value.citytile == CITYT_LAST) {
      break;
    } else {
      static char *tile_property = NULL;

      switch (preq->source.value.citytile) {
      case CITYT_CENTER:
        tile_property = "city centers";
        break;
      case CITYT_CLAIMED:
        tile_property = "claimed tiles";
        break;
      case CITYT_LAST:
        fc_assert(preq->source.value.citytile != CITYT_LAST);
        break;
      }

      switch (preq->range) {
      case REQ_RANGE_LOCAL:
        if (preq->present) {
          cat_snprintf(buf, bufsz,
                       _("Applies only to %s.\n"), tile_property);
        } else {
          cat_snprintf(buf, bufsz,
                       _("Does not apply to %s.\n"), tile_property);
        }
        return TRUE;
      case REQ_RANGE_CADJACENT:
        if (preq->present) {
          cat_snprintf(buf, bufsz, _("Applies only to %s and "
                                     "cardinally adjacent tiles.\n"),
                       tile_property);
        } else {
          cat_snprintf(buf, bufsz, _("Does not apply to %s or "
                                     "cardinally adjacent tiles.\n"),
                       tile_property);
        }
        return TRUE;
      case REQ_RANGE_ADJACENT:
        if (preq->present) {
          cat_snprintf(buf, bufsz, _("Applies only to %s and "
                                     "adjacent tiles.\n"), tile_property);
        } else {
          cat_snprintf(buf, bufsz, _("Does not apply to %s or "
                                     "adjacent tiles.\n"), tile_property);
        }
        return TRUE;
      case REQ_RANGE_CITY:
      case REQ_RANGE_TRADEROUTE:
      case REQ_RANGE_CONTINENT:
      case REQ_RANGE_PLAYER:
      case REQ_RANGE_TEAM:
      case REQ_RANGE_ALLIANCE:
      case REQ_RANGE_WORLD:
      case REQ_RANGE_COUNT:
        /* Not supported. */
        break;
      }
    }

  case VUT_COUNT:
    break;
  }

  {
    char text[256];

    log_error("%s requirement %s in range %d is not supported in helpdata.c.",
              preq->present ? "Present" : "Absent",
              universal_name_translation(&preq->source, text, sizeof(text)),
              preq->range);
  }

  return FALSE;
}

/****************************************************************************
  Append text to 'buf' if the given requirements list 'subjreqs' contains
  'psource', implying that ability to build the subject 'subjstr' is
  affected by 'psource'.
  'strs' is an array of (possibly i18n-qualified) format strings covering
  the various cases where additional requirements apply.
****************************************************************************/
static void insert_allows_single(struct universal *psource,
                                 struct requirement_vector *psubjreqs,
                                 const char *subjstr,
                                 const char *const *strs,
                                 char *buf, size_t bufsz) {
  struct strvec *coreqs = strvec_new();
  struct strvec *conoreqs = strvec_new();
  struct astring coreqstr = ASTRING_INIT;
  struct astring conoreqstr = ASTRING_INIT;
  char buf2[bufsz];

  /* FIXME: show other data like range and survives. */

  requirement_vector_iterate(psubjreqs, req) {
    if (!req->quiet && are_universals_equal(psource, &req->source)) {
      if (req->present) {
        /* psource enables the subject, but other sources may
         * also be required (or required to be absent). */
        requirement_vector_iterate(psubjreqs, coreq) {
          if (!coreq->quiet && !are_universals_equal(psource, &coreq->source)) {
            universal_name_translation(&coreq->source, buf2, sizeof(buf2));
            strvec_append(coreq->present ? coreqs : conoreqs, buf2);
          }
        } requirement_vector_iterate_end;

        if (0 < strvec_size(coreqs)) {
          if (0 < strvec_size(conoreqs)) {
            cat_snprintf(buf, bufsz,
                         Q_(strs[0]), /* "Allows %s (with %s but no %s)." */
                         subjstr,
                         strvec_to_and_list(coreqs, &coreqstr),
                         strvec_to_or_list(conoreqs, &conoreqstr));
          } else {
            cat_snprintf(buf, bufsz,
                         Q_(strs[1]), /* "Allows %s (with %s)." */
                         subjstr,
                         strvec_to_and_list(coreqs, &coreqstr));
          }
        } else {
          if (0 < strvec_size(conoreqs)) {
            cat_snprintf(buf, bufsz,
                         Q_(strs[2]), /* "Allows %s (absent %s)." */
                         subjstr,
                         strvec_to_and_list(conoreqs, &conoreqstr));
          } else {
            cat_snprintf(buf, bufsz,
                         Q_(strs[3]), /* "Allows %s." */
                         subjstr);
          }
        }
      } else {
        /* psource can, on its own, prevent the subject. */
        cat_snprintf(buf, bufsz,
                     Q_(strs[4]), /* "Prevents %s." */
                     subjstr);
      }
      cat_snprintf(buf, bufsz, "\n");
    }
  } requirement_vector_iterate_end;

  strvec_destroy(coreqs);
  strvec_destroy(conoreqs);
  astr_free(&coreqstr);
  astr_free(&conoreqstr);
}

/****************************************************************************
  Generate text for what this requirement source allows.  Something like

    "Allows Communism (with University).\n"
    "Allows Mfg. Plant (with Factory).\n"
    "Allows Library (absent Fundamentalism).\n"
    "Prevents Harbor.\n"

  This should be called to generate helptext for every possible source
  type.  Note this doesn't handle effects but rather requirements to
  create/maintain things (currently only building/government reqs).

  NB: This function overwrites any existing buffer contents by writing the
  generated text to the start of the given 'buf' pointer (i.e. it does
  NOT append like cat_snprintf).
****************************************************************************/
static void insert_allows(struct universal *psource,
			  char *buf, size_t bufsz)
{
  buf[0] = '\0';

  governments_iterate(pgov) {
    static const char *const govstrs[] = {
      /* TRANS: First %s is a government name. */
      N_("?gov:* Allows %s (with %s but no %s)."),
      /* TRANS: First %s is a government name. */
      N_("?gov:* Allows %s (with %s)."),
      /* TRANS: First %s is a government name. */
      N_("?gov:* Allows %s (absent %s)."),
      /* TRANS: %s is a government name. */
      N_("?gov:* Allows %s."),
      /* TRANS: %s is a government name. */
      N_("?gov:* Prevents %s.")
    };
    insert_allows_single(psource, &pgov->reqs,
                         government_name_translation(pgov), govstrs,
                         buf, bufsz);
  } governments_iterate_end;

  improvement_iterate(pimprove) {
    static const char *const imprstrs[] = {
      /* TRANS: First %s is a building name. */
      N_("?improvement:* Allows %s (with %s but no %s)."),
      /* TRANS: First %s is a building name. */
      N_("?improvement:* Allows %s (with %s)."),
      /* TRANS: First %s is a building name. */
      N_("?improvement:* Allows %s (absent %s)."),
      /* TRANS: %s is a building name. */
      N_("?improvement:* Allows %s."),
      /* TRANS: %s is a building name. */
      N_("?improvement:* Prevents %s.")
    };
    insert_allows_single(psource, &pimprove->reqs,
                         improvement_name_translation(pimprove), imprstrs,
                         buf, bufsz);
  } improvement_iterate_end;
}

/****************************************************************
  Allocate and initialize new help item
*****************************************************************/
static struct help_item *new_help_item(int type)
{
  struct help_item *pitem;
  
  pitem = fc_malloc(sizeof(struct help_item));
  pitem->topic = NULL;
  pitem->text = NULL;
  pitem->type = type;
  return pitem;
}

/****************************************************************
 for help_list_sort(); sort by topic via compare_strings()
 (sort topics with more leading spaces after those with fewer)
*****************************************************************/
static int help_item_compar(const struct help_item *const *ppa,
                            const struct help_item *const *ppb)
{
  const struct help_item *ha, *hb;
  char *ta, *tb;
  ha = *ppa;
  hb = *ppb;
  for (ta = ha->topic, tb = hb->topic; *ta != '\0' && *tb != '\0'; ta++, tb++) {
    if (*ta != ' ') {
      if (*tb == ' ') return -1;
      break;
    } else if (*tb != ' ') {
      if (*ta == ' ') return 1;
      break;
    }
  }
  return compare_strings(ta, tb);
}

/****************************************************************
  pplayer may be NULL.
*****************************************************************/
void boot_help_texts(void)
{
  static bool booted = FALSE;

  struct section_file *sf;
  const char *filename;
  struct help_item *pitem;
  int i;
  struct section_list *sec;
  const char **paras;
  size_t npara;
  char long_buffer[64000]; /* HACK: this may be overrun. */

  check_help_nodes_init();

  /* need to do something like this or bad things happen */
  popdown_help_dialog();

  if (!booted) {
    log_verbose("Booting help texts");
  } else {
    /* free memory allocated last time booted */
    free_help_texts();
    log_verbose("Rebooting help texts");
  }

  filename = fileinfoname(get_data_dirs(), "helpdata.txt");
  if (!filename) {
    log_error("Did not read help texts");
    return;
  }
  /* after following call filename may be clobbered; use sf->filename instead */
  if (!(sf = secfile_load(filename, FALSE))) {
    /* this is now unlikely to happen */
    log_error("failed reading help-texts from '%s':\n%s", filename,
              secfile_error());
    return;
  }

  sec = secfile_sections_by_name_prefix(sf, "help_");

  if (NULL != sec) {
    section_list_iterate(sec, psection) {
      char help_text_buffer[MAX_LEN_PACKET];
      const char *sec_name = section_name(psection);
      const char *gen_str = secfile_lookup_str(sf, "%s.generate", sec_name);
      
      if (gen_str) {
        enum help_page_type current_type = HELP_ANY;
        int level = strspn(gen_str, " ");
        gen_str += level;
        if (!booted) {
          continue; /* on initial boot data tables are empty */
        }
        for (i = 2; help_type_names[i]; i++) {
          if (strcmp(gen_str, help_type_names[i]) == 0) {
            current_type = i;
            break;
          }
        }
        if (current_type == HELP_ANY) {
          log_error("bad help-generate category \"%s\"", gen_str);
          continue;
        }
        {
          /* Note these should really fill in pitem->text from auto-gen
             data instead of doing it later on the fly, but I don't want
             to change that now.  --dwp
          */
          char name[2048];
          struct help_list *category_nodes = help_list_new();

          switch (current_type) {
          case HELP_UNIT:
            unit_type_iterate(punittype) {
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          utype_name_translation(punittype));
              pitem->topic = fc_strdup(name);
              pitem->text = fc_strdup("");
              help_list_append(category_nodes, pitem);
            } unit_type_iterate_end;
            break;
          case HELP_TECH:
            advance_index_iterate(A_FIRST, advi) {
              if (valid_advance_by_number(advi)) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            advance_name_translation(advance_by_number(advi)));
                pitem->topic = fc_strdup(name);
                pitem->text = fc_strdup("");
                help_list_append(category_nodes, pitem);
              }
            } advance_index_iterate_end;
            break;
          case HELP_TERRAIN:
            terrain_type_iterate(pterrain) {
              if (0 != strlen(terrain_rule_name(pterrain))) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            terrain_name_translation(pterrain));
                pitem->topic = fc_strdup(name);
                pitem->text = fc_strdup("");
                help_list_append(category_nodes, pitem);
              }
            } terrain_type_iterate_end;
            break;
          case HELP_EXTRA:
            {
              const char **cats;
              size_t ncats;
              cats = secfile_lookup_str_vec(sf, &ncats,
                                            "%s.categories", sec_name);
              extra_type_iterate(pextra) {
                /* If categories not specified, don't filter */
                if (cats) {
                  bool include = FALSE;
                  const char *cat = extra_category_name(pextra->category);
                  int ci;

                  for (ci = 0; ci < ncats; ci++) {
                    if (fc_strcasecmp(cats[ci], cat) == 0) {
                      include = TRUE;
                      break;
                    }
                  }
                  if (!include) {
                    continue;
                  }
                }
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            extra_name_translation(pextra));
                pitem->topic = fc_strdup(name);
                pitem->text = fc_strdup("");
                help_list_append(category_nodes, pitem);
              } extra_type_iterate_end;
              FC_FREE(cats);
            }
            break;
          case HELP_SPECIALIST:
            specialist_type_iterate(sp) {
              struct specialist *pspec = specialist_by_number(sp);
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          specialist_plural_translation(pspec));
              pitem->topic = fc_strdup(name);
              pitem->text = fc_strdup("");
              help_list_append(category_nodes, pitem);
            } specialist_type_iterate_end;
            break;
          case HELP_GOVERNMENT:
            governments_iterate(gov) {
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          government_name_translation(gov));
              pitem->topic = fc_strdup(name);
              pitem->text = fc_strdup("");
              help_list_append(category_nodes, pitem);
            } governments_iterate_end;
            break;
          case HELP_IMPROVEMENT:
            improvement_iterate(pimprove) {
              if (valid_improvement(pimprove) && !is_great_wonder(pimprove)) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            improvement_name_translation(pimprove));
                pitem->topic = fc_strdup(name);
                pitem->text = fc_strdup("");
                help_list_append(category_nodes, pitem);
              }
            } improvement_iterate_end;
            break;
          case HELP_WONDER:
            improvement_iterate(pimprove) {
              if (valid_improvement(pimprove) && is_great_wonder(pimprove)) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            improvement_name_translation(pimprove));
                pitem->topic = fc_strdup(name);
                pitem->text = fc_strdup("");
                help_list_append(category_nodes, pitem);
              }
            } improvement_iterate_end;
            break;
          case HELP_RULESET:
            {
              int desc_len;
              int len;

              pitem = new_help_item(HELP_RULESET);
              /*           pitem->topic = fc_strdup(_(game.control.name)); */
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          Q_(HELP_RULESET_ITEM));
              pitem->topic = fc_strdup(name);
              if (game.ruleset_description != NULL) {
                desc_len = strlen("\n\n") + strlen(game.ruleset_description);
              } else {
                desc_len = 0;
              }
              if (game.ruleset_summary != NULL) {
                if (game.control.version[0] != '\0') {
                  len = strlen(_(game.control.name))
                    + strlen(" ")
                    + strlen(game.control.version)
                    + strlen("\n\n")
                    + strlen(_(game.ruleset_summary))
                    + 1;

                  pitem->text = fc_malloc(len + desc_len);
                  fc_snprintf(pitem->text, len, "%s %s\n\n%s",
                              _(game.control.name), game.control.version,
                              _(game.ruleset_summary));
                } else {
                  len = strlen(_(game.control.name))
                    + strlen("\n\n")
                    + strlen(_(game.ruleset_summary))
                    + 1;

                  pitem->text = fc_malloc(len + desc_len);
                  fc_snprintf(pitem->text, len, "%s\n\n%s",
                              _(game.control.name), _(game.ruleset_summary));
                }
              } else {
                const char *nodesc = _("Current ruleset contains no summary.");

                if (game.control.version[0] != '\0') {
                  len = strlen(_(game.control.name))
                    + strlen(" ")
                    + strlen(game.control.version)
                    + strlen("\n\n")
                    + strlen(nodesc)
                    + 1;

                  pitem->text = fc_malloc(len + desc_len);
                  fc_snprintf(pitem->text, len, "%s %s\n\n%s",
                              _(game.control.name), game.control.version,
                              nodesc);
                } else {
                  len = strlen(_(game.control.name))
                    + strlen("\n\n")
                    + strlen(nodesc)
                    + 1;

                  pitem->text = fc_malloc(len + desc_len);
                  fc_snprintf(pitem->text, len, "%s\n\n%s",
                              _(game.control.name),
                              nodesc);
                }
              }
              if (game.ruleset_description != NULL) {
                fc_strlcat(pitem->text, "\n\n", len + desc_len);
                fc_strlcat(pitem->text, game.ruleset_description, len + desc_len);
              }
              help_list_append(help_nodes, pitem);
            }
            break;
          case HELP_TILESET:
            {
              int desc_len;
              int len;
              const char *ts_name = tileset_name(tileset);
              const char *version = tileset_version(tileset);
              const char *summary = tileset_summary(tileset);
              const char *description = tileset_description(tileset);

              pitem = new_help_item(HELP_TILESET);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          Q_(HELP_TILESET_ITEM));
              pitem->topic = fc_strdup(name);
              if (description != NULL) {
                desc_len = strlen("\n\n") + strlen(description);
              } else {
                desc_len = 0;
              }
              if (summary != NULL) {
                if (version[0] != '\0') {
                  len = strlen(_(ts_name))
                    + strlen(" ")
                    + strlen(version)
                    + strlen("\n\n")
                    + strlen(_(summary))
                    + 1;

                  pitem->text = fc_malloc(len + desc_len);
                  fc_snprintf(pitem->text, len, "%s %s\n\n%s",
                              _(ts_name), version, _(summary));
                } else {
                  len = strlen(_(ts_name))
                    + strlen("\n\n")
                    + strlen(_(summary))
                    + 1;

                  pitem->text = fc_malloc(len + desc_len);
                  fc_snprintf(pitem->text, len, "%s\n\n%s",
                              _(ts_name), _(summary));
                }
              } else {
                const char *nodesc = _("Current tileset contains no summary.");

                if (version[0] != '\0') {
                  len = strlen(_(ts_name))
                    + strlen(" ")
                    + strlen(version)
                    + strlen("\n\n")
                    + strlen(nodesc)
                    + 1;

                  pitem->text = fc_malloc(len + desc_len);
                  fc_snprintf(pitem->text, len, "%s %s\n\n%s",
                              _(ts_name), version,
                              nodesc);
                } else {
                  len = strlen(_(ts_name))
                    + strlen("\n\n")
                    + strlen(nodesc)
                    + 1;

                  pitem->text = fc_malloc(len + desc_len);
                  fc_snprintf(pitem->text, len, "%s\n\n%s",
                              _(ts_name),
                              nodesc);
                }
              }
              if (description != NULL) {
                fc_strlcat(pitem->text, "\n\n", len + desc_len);
                fc_strlcat(pitem->text, description, len + desc_len);
              }
              help_list_append(help_nodes, pitem);
            }
            break;
          case HELP_NATIONS:
            nations_iterate(pnation) {
              if (client_state() < C_S_RUNNING
                  || show_help_for_nation(pnation)) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            nation_plural_translation(pnation));
                pitem->topic = fc_strdup(name);
                pitem->text = fc_strdup("");
                help_list_append(category_nodes, pitem);
              }
            } nations_iterate_end;
            break;
	  case HELP_MULTIPLIER:
            multipliers_iterate(pmul) {
              help_text_buffer[0] = '\0';
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          name_translation_get(&pmul->name));
              pitem->topic = fc_strdup(name);
              if (pmul->helptext) {
                const char *sep = "";
                strvec_iterate(pmul->helptext, text) {
                  cat_snprintf(help_text_buffer, sizeof(help_text_buffer),
                               "%s%s", sep, text);
                  sep = "\n\n";
                } strvec_iterate_end;
              }
              pitem->text = fc_strdup(help_text_buffer);
              help_list_append(help_nodes, pitem);
            } multipliers_iterate_end;
            break;
          default:
            log_error("Bad current_type: %d.", current_type);
            break;
          }
          help_list_sort(category_nodes, help_item_compar);
          help_list_iterate(category_nodes, ptmp) {
            help_list_append(help_nodes, ptmp);
          } help_list_iterate_end;
          help_list_destroy(category_nodes);
          continue;
        }
      }
      
      /* It wasn't a "generate" node: */
      
      pitem = new_help_item(HELP_TEXT);
      pitem->topic = fc_strdup(Q_(secfile_lookup_str(sf, "%s.name",
                                                     sec_name)));

      paras = secfile_lookup_str_vec(sf, &npara, "%s.text", sec_name);

      long_buffer[0] = '\0';
      for (i=0; i<npara; i++) {
        bool inserted;
        const char *para = paras[i];
        if(strncmp(para, "$", 1)==0) {
          inserted =
            insert_generated_text(long_buffer, sizeof(long_buffer), para+1);
        } else {
          sz_strlcat(long_buffer, _(para));
          inserted = TRUE;
        }
        if (inserted && i!=npara-1) {
          sz_strlcat(long_buffer, "\n\n");
        }
      }
      free(paras);
      paras = NULL;
      pitem->text=fc_strdup(long_buffer);
      help_list_append(help_nodes, pitem);
    } section_list_iterate_end;

    section_list_destroy(sec);
  }

  secfile_check_unused(sf);
  secfile_destroy(sf);
  booted = TRUE;
  log_verbose("Booted help texts ok");
}

/****************************************************************
  The following few functions are essentially wrappers for the
  help_nodes help_list.  This allows us to avoid exporting the
  help_list, and instead only access it through a controlled
  interface.
*****************************************************************/

/****************************************************************
  Number of help items.
*****************************************************************/
int num_help_items(void)
{
  check_help_nodes_init();
  return help_list_size(help_nodes);
}

/****************************************************************
  Return pointer to given help_item.
  Returns NULL for 1 past end.
  Returns NULL and prints error message for other out-of bounds.
*****************************************************************/
const struct help_item *get_help_item(int pos)
{
  int size;
  
  check_help_nodes_init();
  size = help_list_size(help_nodes);
  if (pos < 0 || pos > size) {
    log_error("Bad index %d to get_help_item (size %d)", pos, size);
    return NULL;
  }
  if (pos == size) {
    return NULL;
  }
  return help_list_get(help_nodes, pos);
}

/****************************************************************
  Find help item by name and type.
  Returns help item, and sets (*pos) to position in list.
  If no item, returns pointer to static internal item with
  some faked data, and sets (*pos) to -1.
*****************************************************************/
const struct help_item*
get_help_item_spec(const char *name, enum help_page_type htype, int *pos)
{
  int idx;
  const struct help_item *pitem = NULL;
  static struct help_item vitem; /* v = virtual */
  static char vtopic[128];
  static char vtext[256];

  check_help_nodes_init();
  idx = 0;
  help_list_iterate(help_nodes, ptmp) {
    char *p=ptmp->topic;
    while (*p == ' ') {
      p++;
    }
    if(strcmp(name, p)==0 && (htype==HELP_ANY || htype==ptmp->type)) {
      pitem = ptmp;
      break;
    }
    idx++;
  }
  help_list_iterate_end;
  
  if(!pitem) {
    idx = -1;
    vitem.topic = vtopic;
    sz_strlcpy(vtopic, name);
    vitem.text = vtext;
    if(htype==HELP_ANY || htype==HELP_TEXT) {
      fc_snprintf(vtext, sizeof(vtext),
		  _("Sorry, no help topic for %s.\n"), vitem.topic);
      vitem.type = HELP_TEXT;
    } else {
      fc_snprintf(vtext, sizeof(vtext),
		  _("Sorry, no help topic for %s.\n"
		    "This page was auto-generated.\n\n"),
		  vitem.topic);
      vitem.type = htype;
    }
    pitem = &vitem;
  }
  *pos = idx;
  return pitem;
}

/****************************************************************
  Start iterating through help items;
  that is, reset iterator to start position.
  (Could iterate using get_help_item(), but that would be
  less efficient due to scanning to find pos.)
*****************************************************************/
void help_iter_start(void)
{
  check_help_nodes_init();
  help_nodes_iterator = help_list_head(help_nodes);
}

/****************************************************************
  Returns next help item; after help_iter_start(), this is
  the first item.  At end, returns NULL.
*****************************************************************/
const struct help_item *help_iter_next(void)
{
  const struct help_item *pitem;
  
  check_help_nodes_init();
  pitem = help_list_link_data(help_nodes_iterator);
  if (pitem) {
    help_nodes_iterator = help_list_link_next(help_nodes_iterator);
  }

  return pitem;
}


/****************************************************************
  FIXME:
  Also, in principle these could be auto-generated once, inserted
  into pitem->text, and then don't need to keep re-generating them.
  Only thing to be careful of would be changeable data, but don't
  have that here (for ruleset change or spacerace change must
  re-boot helptexts anyway).  Eg, genuinely dynamic information
  which could be useful would be if help system said which wonders
  have been built (or are being built and by who/where?)
*****************************************************************/

/**************************************************************************
  Write dynamic text for buildings (including wonders).  This includes
  the ruleset helptext as well as any automatically generated text.

  pplayer may be NULL.
  user_text, if non-NULL, will be appended to the text.
**************************************************************************/
char *helptext_building(char *buf, size_t bufsz, struct player *pplayer,
                        const char *user_text, struct impr_type *pimprove)
{
  bool reqs = FALSE;
  struct universal source = {
    .kind = VUT_IMPROVEMENT,
    .value = {.building = pimprove}
  };

  fc_assert_ret_val(NULL != buf && 0 < bufsz, NULL);
  buf[0] = '\0';

  if (NULL == pimprove) {
    return buf;
  }

  if (NULL != pimprove->helptext) {
    strvec_iterate(pimprove->helptext, text) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(text));
    } strvec_iterate_end;
  }

  /* Add requirement text for improvement itself */
  requirement_vector_iterate(&pimprove->reqs, preq) {
    if (insert_requirement(buf, bufsz, pplayer, preq)) {
      reqs = TRUE;
    }
  } requirement_vector_iterate_end;
  if (reqs) {
    fc_strlcat(buf, "\n", bufsz);
  }

  requirement_vector_iterate(&pimprove->obsolete_by, pobs) {
    if (VUT_ADVANCE == pobs->source.kind && pobs->present) {
      cat_snprintf(buf, bufsz,
                   _("* The discovery of %s will make %s obsolete.\n"),
                   advance_name_translation(pobs->source.value.advance),
                   improvement_name_translation(pimprove));
    }
    if (VUT_IMPROVEMENT == pobs->source.kind && pobs->present) {
      cat_snprintf(buf, bufsz,
                   _("* The presence of %s in the city will make %s "
                     "obsolete.\n"),
                     improvement_name_translation(pobs->source.value.building),
                     improvement_name_translation(pimprove));
    }
  } requirement_vector_iterate_end;

  if (is_small_wonder(pimprove)) {
    cat_snprintf(buf, bufsz,
                 _("* A 'small wonder': at most one of your cities may "
                   "possess this improvement.\n"));
  }
  /* (Great wonders are in their own help section explaining their
   * uniqueness, so we don't mention it here.) */

  if (building_has_effect(pimprove, EFT_ENABLE_NUKE)
      && num_role_units(action_get_role(ACTION_NUKE)) > 0) {
    struct unit_type *u = get_role_unit(action_get_role(ACTION_NUKE), 0);

    cat_snprintf(buf, bufsz,
		 /* TRANS: 'Allows all players with knowledge of atomic
		  * power to build nuclear units.' */
		 _("* Allows all players with knowledge of %s "
		   "to build %s units.\n"),
                 advance_name_translation(u->require_advance),
		 utype_name_translation(u));
  }

  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));

  unit_type_iterate(u) {
    if (u->need_improvement == pimprove) {
      if (A_NEVER != u->require_advance) {
	cat_snprintf(buf, bufsz, _("* Allows %s (with %s).\n"),
		     utype_name_translation(u),
                     advance_name_translation(u->require_advance));
      } else {
	cat_snprintf(buf, bufsz, _("* Allows %s.\n"),
		     utype_name_translation(u));
      }
    }
  } unit_type_iterate_end;

  {
    int i;

    for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
      Impr_type_id n = game.rgame.global_init_buildings[i];
      if (n == B_LAST) {
        break;
      } else if (improvement_by_number(n) == pimprove) {
        cat_snprintf(buf, bufsz,
                     _("* All players start with this improvement in their "
                       "first city.\n"));
        break;
      }
    }
  }

  /* Assume no-one will set the same building in both global and nation
   * init_buildings... */
  nations_iterate(pnation) {
    int i;

    /* Avoid mentioning nations not in current set. */
    if (!show_help_for_nation(pnation)) {
      continue;
    }
    for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
      Impr_type_id n = pnation->init_buildings[i];
      if (n == B_LAST) {
        break;
      } else if (improvement_by_number(n) == pimprove) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a nation plural */
                     _("* The %s start with this improvement in their "
                       "first city.\n"), nation_plural_translation(pnation));
        break;
      }
    }
  } nations_iterate_end;

  if (improvement_has_flag(pimprove, IF_SAVE_SMALL_WONDER)) {
    cat_snprintf(buf, bufsz,
                 /* TRANS: don't translate 'savepalace' */
                 _("* If you lose the city containing this improvement, "
                   "it will be rebuilt for free in another of your cities "
                   "(if the 'savepalace' server setting is enabled).\n"));
  }

  if (user_text && user_text[0] != '\0') {
    cat_snprintf(buf, bufsz, "\n\n%s", user_text);
  }
  return buf;
}

/****************************************************************
  Is unit type ever able to build an extra
*****************************************************************/
static bool help_is_extra_buildable(struct extra_type *pextra,
                                    struct unit_type *ptype)
{
  if (!pextra->buildable) {
    return FALSE;
  }

  return are_reqs_active(NULL, NULL, NULL, NULL, NULL,
                         NULL, ptype, NULL, NULL, NULL, &pextra->reqs,
                         RPT_POSSIBLE);
}

/****************************************************************
  Is unit type ever able to clean out an extra
*****************************************************************/
static bool help_is_extra_cleanable(struct extra_type *pextra,
                                    struct unit_type *ptype)
{
  return are_reqs_active(NULL, NULL, NULL, NULL, NULL,
                         NULL, ptype, NULL, NULL, NULL, &pextra->rmreqs,
                         RPT_POSSIBLE);
}

/****************************************************************
  Append misc dynamic text for units.
  Transport capacity, unit flags, fuel.

  pplayer may be NULL.
*****************************************************************/
char *helptext_unit(char *buf, size_t bufsz, struct player *pplayer,
		    const char *user_text, struct unit_type *utype)
{
  bool has_vet_levels;
  int flagid;
  struct unit_class *pclass;

  fc_assert_ret_val(NULL != buf && 0 < bufsz && NULL != user_text, NULL);

  if (!utype) {
    log_error("Unknown unit!");
    fc_strlcpy(buf, user_text, bufsz);
    return buf;
  }

  has_vet_levels = utype_veteran_levels(utype) > 1;

  buf[0] = '\0';

  pclass = utype_class(utype);
  cat_snprintf(buf, bufsz,
               _("* Belongs to %s unit class."),
               uclass_name_translation(pclass));
  if (NULL != pclass->helptext) {
    strvec_iterate(pclass->helptext, text) {
      cat_snprintf(buf, bufsz, "\n%s\n", _(text));
    } strvec_iterate_end;
  } else {
    CATLSTR(buf, bufsz, "\n");
  }
  if (uclass_has_flag(pclass, UCF_CAN_OCCUPY_CITY)
      && !utype_has_flag(utype, UTYF_CIVILIAN)) {
    CATLSTR(buf, bufsz, _("  * Can occupy empty enemy cities.\n"));
  }
  if (!uclass_has_flag(pclass, UCF_TERRAIN_SPEED)) {
    CATLSTR(buf, bufsz, _("  * Speed is not affected by terrain.\n"));
  }
  if (!uclass_has_flag(pclass, UCF_TERRAIN_DEFENSE)) {
    CATLSTR(buf, bufsz, _("  * Does not get defense bonuses from terrain.\n"));
  }
  if (!uclass_has_flag(pclass, UCF_ZOC)) {
    CATLSTR(buf, bufsz, _("  * Not subject to zones of control.\n"));
  } else if (!utype_has_flag(utype, UTYF_IGZOC)) {
    CATLSTR(buf, bufsz, _("  * Subject to zones of control.\n"));
  }
  if (uclass_has_flag(pclass, UCF_DAMAGE_SLOWS)) {
    CATLSTR(buf, bufsz, _("  * Slowed down while damaged.\n"));
  }
  if (uclass_has_flag(pclass, UCF_MISSILE)) {
    CATLSTR(buf, bufsz, _("  * Gets used up in making an attack.\n"));
  }
  if (uclass_has_flag(pclass, UCF_CAN_FORTIFY)
      && !utype_has_flag(utype, UTYF_CANT_FORTIFY)) {
    if (utype->defense_strength > 0) {
      CATLSTR(buf, bufsz,
              /* xgettext:no-c-format */
              _("  * Gets a 50% defensive bonus while in cities.\n"));
      CATLSTR(buf, bufsz,
              /* xgettext:no-c-format */
              _("  * May fortify, granting a 50% defensive bonus when not in "
                "a city.\n"));
    } else {
      CATLSTR(buf, bufsz,
              _("  * May fortify to stay put.\n"));
    }
  }
  if (uclass_has_flag(pclass, UCF_UNREACHABLE)) {
    CATLSTR(buf, bufsz,
	    _("  * Is unreachable. Most units cannot attack this one.\n"));
  }
  if (uclass_has_flag(pclass, UCF_CAN_PILLAGE)) {
    CATLSTR(buf, bufsz,
	    _("  * Can pillage tile improvements.\n"));
  }
  if (uclass_has_flag(pclass, UCF_DOESNT_OCCUPY_TILE)
      && !utype_has_flag(utype, UTYF_CIVILIAN)) {
    CATLSTR(buf, bufsz,
	    _("  * Doesn't prevent enemy cities from working the tile it's on.\n"));
  }
  if (can_attack_non_native(utype)) {
    CATLSTR(buf, bufsz,
	    _("  * Can attack units on non-native tiles.\n"));
  }
  /* Must use flag to distinguish from UTYF_MARINES text. */
  if (utype->attack_strength > 0
      && uclass_has_flag(pclass, UCF_ATT_FROM_NON_NATIVE)) {
    CATLSTR(buf, bufsz,
            _("  * Can launch attack from non-native tiles.\n"));
  }
  if (uclass_has_flag(pclass, UCF_AIRLIFTABLE)) {
    CATLSTR(buf, bufsz,
            _("  * Can be airlifted from a suitable city.\n"));
  }

  /* The unit's combat bonuses. Won't mention that another unit type has a
   * combat bonus against this unit type. Doesn't handle complex cases like
   * when a unit type has multiple combat bonuses of the same kind. */
  combat_bonus_list_iterate(utype->bonuses, cbonus) {
    const char *against[utype_count()];
    int targets = 0;

    if (cbonus->quiet) {
      /* Handled in the help text of the ruleset. */
      continue;
    }

    /* Find the unit types of the bonus targets. */
    unit_type_iterate(utype2) {
      if (utype_has_flag(utype2, cbonus->flag)) {
        against[targets++] = utype_name_translation(utype2);
      }
    } unit_type_iterate_end;

    if (targets > 0) {
      struct astring list = ASTRING_INIT;

      switch (cbonus->type) {
      case CBONUS_DEFENSE_MULTIPLIER:
        cat_snprintf(buf, bufsz,
                     /* TRANS: multipied by ... or-list of unit types */
                     _("* %dx defense bonus if attacked by %s.\n"),
                     cbonus->value + 1,
                     astr_build_or_list(&list, against, targets));
        break;
      case CBONUS_DEFENSE_DIVIDER:
        cat_snprintf(buf, bufsz,
                     /* TRANS: defense divider ... or-list of unit types */
                     _("* reduces target's defense to 1 / %d when "
                       "attacking %s.\n"),
                     cbonus->value + 1,
                     astr_build_or_list(&list, against, targets));
        break;
      case CBONUS_FIREPOWER1:
        cat_snprintf(buf, bufsz,
                     /* TRANS: or-list of unit types */
                     _("* reduces target's fire power to 1 when "
                       "attacking %s.\n"),
                     astr_build_and_list(&list, against, targets));
        break;
      }

      astr_free(&list);
    }
  } combat_bonus_list_iterate_end;

  if (utype->need_improvement) {
    cat_snprintf(buf, bufsz,
                 _("* Can only be built if there is %s in the city.\n"),
                 improvement_name_translation(utype->need_improvement));
  }

  if (utype->need_government) {
    cat_snprintf(buf, bufsz,
                 _("* Can only be built with %s as government.\n"),
                 government_name_translation(utype->need_government));
  }

  if (utype_has_flag(utype, UTYF_CANESCAPE)) {
    CATLSTR(buf, bufsz, _("* Can escape once stack defender is lost.\n"));
  }
  if (utype_has_flag(utype, UTYF_CANKILLESCAPING)) {
    CATLSTR(buf, bufsz, _("* Can pursue escaping units and kill them.\n"));
  }

  if (utype_has_flag(utype, UTYF_NOBUILD)) {
    CATLSTR(buf, bufsz, _("* May not be built in cities.\n"));
  }
  if (utype_has_flag(utype, UTYF_BARBARIAN_ONLY)) {
    CATLSTR(buf, bufsz, _("* Only barbarians may build this.\n"));
  }
  if (utype_has_flag(utype, UTYF_NEWCITY_GAMES_ONLY)) {
    CATLSTR(buf, bufsz, _("* This may be built only in games where new cities are allowed.\n"));
    if (game.scenario.prevent_new_cities) {
      CATLSTR(buf, bufsz, _("  - New cities are not allowed in current game.\n"));
    } else {
      CATLSTR(buf, bufsz, _("  - New cities are allowed in current game.\n"));
    }
  }
  nations_iterate(pnation) {
    int i, count = 0;

    /* Avoid mentioning nations not in current set. */
    if (!show_help_for_nation(pnation)) {
      continue;
    }
    for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
      if (!pnation->init_units[i]) {
        break;
      } else if (pnation->init_units[i] == utype) {
        count++;
      }
    }
    if (count > 0) {
      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is a nation plural */
                   PL_("* The %s start the game with %d of these units.\n",
                       "* The %s start the game with %d of these units.\n",
                       count),
                   nation_plural_translation(pnation), count);
    }
  } nations_iterate_end;
  {
    const char *types[utype_count()];
    int i = 0;
    unit_type_iterate(utype2) {
      if (utype2->converted_to == utype) {
        types[i++] = utype_name_translation(utype2);
      }
    } unit_type_iterate_end;
    if (i > 0) {
      struct astring list = ASTRING_INIT;
      astr_build_or_list(&list, types, i);
      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is a list of unit types separated by "or". */
                   _("* May be obtained by conversion of %s.\n"),
                   astr_str(&list));
      astr_free(&list);
    }
  }
  if (NULL != utype->converted_to) {
    cat_snprintf(buf, bufsz,
                 /* TRANS: %s is a unit type. "MP" = movement points. */
                 PL_("* May be converted into %s (takes %d MP).\n",
                     "* May be converted into %s (takes %d MP).\n",
                     utype->convert_time),
                 utype_name_translation(utype->converted_to),
                 utype->convert_time);
  }
  if (utype_has_flag(utype, UTYF_NOHOME)) {
    CATLSTR(buf, bufsz, _("* Never has a home city.\n"));
  }
  if (utype_has_flag(utype, UTYF_GAMELOSS)) {
    CATLSTR(buf, bufsz, _("* Losing this unit will lose you the game!\n"));
  }
  if (utype_has_flag(utype, UTYF_UNIQUE)) {
    CATLSTR(buf, bufsz,
	    _("* Each player may only have one of this type of unit.\n"));
  }
  for (flagid = UTYF_USER_FLAG_1 ; flagid <= UTYF_LAST_USER_FLAG; flagid++) {
    if (utype_has_flag(utype, flagid)) {
      const char *helptxt = unit_type_flag_helptxt(flagid);

      if (helptxt != NULL) {
        CATLSTR(buf, bufsz, Q_("?bullet:* "));
        CATLSTR(buf, bufsz, _(helptxt));
        CATLSTR(buf, bufsz, "\n");
      }
    }
  }
  if (utype->pop_cost > 0) {
    cat_snprintf(buf, bufsz,
                 PL_("* Costs %d population to build.\n",
                     "* Costs %d population to build.\n", utype->pop_cost),
                 utype->pop_cost);
  }
  if (0 < utype->transport_capacity) {
    const char *classes[uclass_count()];
    int i = 0;
    struct astring list = ASTRING_INIT;

    unit_class_iterate(uclass) {
      if (can_unit_type_transport(utype, uclass)) {
        classes[i++] = uclass_name_translation(uclass);
      }
    } unit_class_iterate_end;
    astr_build_or_list(&list, classes, i);

    cat_snprintf(buf, bufsz,
                 /* TRANS: %s is a list of unit classes separated by "or". */
                 PL_("* Can carry and refuel %d %s unit.\n",
                     "* Can carry and refuel up to %d %s units.\n",
                     utype->transport_capacity),
                 utype->transport_capacity, astr_str(&list));
    astr_free(&list);
    if (uclass_has_flag(utype_class(utype), UCF_UNREACHABLE)) {
      /* Document restrictions on when units can load/unload */
      bool has_restricted_load = FALSE, has_unrestricted_load = FALSE,
           has_restricted_unload = FALSE, has_unrestricted_unload = FALSE;
      unit_type_iterate(pcargo) {
        if (can_unit_type_transport(utype, utype_class(pcargo))) {
          if (utype_can_freely_load(pcargo, utype)) {
            has_unrestricted_load = TRUE;
          } else {
            has_restricted_load = TRUE;
          }
          if (utype_can_freely_unload(pcargo, utype)) {
            has_unrestricted_unload = TRUE;
          } else {
            has_restricted_unload = TRUE;
          }
        }
      } unit_type_iterate_end;
      if (has_restricted_load) {
        if (has_unrestricted_load) {
          /* At least one type of cargo can load onto us freely.
           * The specific exceptions will be documented in cargo help. */
          CATLSTR(buf, bufsz,
                  _("  * Some cargo cannot be loaded except in a city or a "
                    "base native to this transport.\n"));
        } else {
          /* No exceptions */
          CATLSTR(buf, bufsz,
                  _("  * Cargo cannot be loaded except in a city or a "
                    "base native to this transport.\n"));
        }
      } /* else, no restricted cargo exists; keep quiet */
      if (has_restricted_unload) {
        if (has_unrestricted_unload) {
          /* At least one type of cargo can unload from us freely. */
          CATLSTR(buf, bufsz,
                  _("  * Some cargo cannot be unloaded except in a city or a "
                    "base native to this transport.\n"));
        } else {
          /* No exceptions */
          CATLSTR(buf, bufsz,
                  _("  * Cargo cannot be unloaded except in a city or a "
                    "base native to this transport.\n"));
        }
      } /* else, no restricted cargo exists; keep quiet */
    }
  }
  if (utype_has_flag(utype, UTYF_TRIREME)) {
    CATLSTR(buf, bufsz, _("* Must stay next to coast.\n"));
  }
  {
    /* Document exceptions to embark/disembark restrictions that we
     * have as cargo. */
    bv_unit_classes embarks, disembarks;
    BV_CLR_ALL(embarks);
    BV_CLR_ALL(disembarks);
    /* Determine which of our transport classes have restrictions in the first
     * place (that is, contain at least one transport which carries at least
     * one type of cargo which is restricted).
     * We'll suppress output for classes not in this set, since this cargo
     * type is not behaving exceptionally in such cases. */
    unit_type_iterate(utrans) {
      const Unit_Class_id trans_class = uclass_index(utype_class(utrans));
      /* Don't waste time repeating checks on classes we've already checked,
       * or weren't under consideration in the first place */
      if (!BV_ISSET(embarks, trans_class)
          && BV_ISSET(utype->embarks, trans_class)) {
        unit_type_iterate(other_cargo) {
          if (can_unit_type_transport(utrans, utype_class(other_cargo))
              && !utype_can_freely_load(other_cargo, utrans)) {
            /* At least one load restriction in transport class, which
             * we aren't subject to */
            BV_SET(embarks, trans_class);
          }
        } unit_type_iterate_end; /* cargo */
      }
      if (!BV_ISSET(disembarks, trans_class)
          && BV_ISSET(utype->disembarks, trans_class)) {
        unit_type_iterate(other_cargo) {
          if (can_unit_type_transport(utrans, utype_class(other_cargo))
              && !utype_can_freely_unload(other_cargo, utrans)) {
            /* At least one load restriction in transport class, which
             * we aren't subject to */
            BV_SET(disembarks, trans_class);
          }
        } unit_type_iterate_end; /* cargo */
      }
    } unit_class_iterate_end; /* transports */

    if (BV_ISSET_ANY(embarks)) {
      /* Build list of embark exceptions */
      const char *eclasses[uclass_count()];
      int i = 0;
      struct astring elist = ASTRING_INIT;

      unit_class_iterate(uclass) {
        if (BV_ISSET(embarks, uclass_index(uclass))) {
          eclasses[i++] = uclass_name_translation(uclass);
        }
      } unit_class_iterate_end;
      astr_build_or_list(&elist, eclasses, i);
      if (BV_ARE_EQUAL(embarks, disembarks)) {
        /* A common case: the list of disembark exceptions is identical */
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a list of unit classes separated
                      * by "or". */
                     _("* May load onto and unload from %s transports even "
                       "when underway.\n"),
                     astr_str(&elist));
      } else {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a list of unit classes separated
                      * by "or". */
                     _("* May load onto %s transports even when underway.\n"),
                     astr_str(&elist));
      }
      astr_free(&elist);
    }
    if (BV_ISSET_ANY(disembarks) && !BV_ARE_EQUAL(embarks, disembarks)) {
      /* Build list of disembark exceptions (if different from embarking) */
      const char *dclasses[uclass_count()];
      int i = 0;
      struct astring dlist = ASTRING_INIT;

      unit_class_iterate(uclass) {
        if (BV_ISSET(disembarks, uclass_index(uclass))) {
          dclasses[i++] = uclass_name_translation(uclass);
        }
      } unit_class_iterate_end;
      astr_build_or_list(&dlist, dclasses, i);
      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is a list of unit classes separated
                    * by "or". */
                   _("* May unload from %s transports even when underway.\n"),
                   astr_str(&dlist));
      astr_free(&dlist);
    }
  }
  if (utype_has_flag(utype, UTYF_SETTLERS)) {
    struct universal for_utype = { .kind = VUT_UTYPE, .value = { .utype = utype }};
    struct astring extras_and = ASTRING_INIT;
    struct strvec *extras_vec = strvec_new();

    /* Roads, rail, mines, irrigation. */
    extra_type_by_cause_iterate(EC_ROAD, pextra) {
      if (help_is_extra_buildable(pextra, utype)) {
        strvec_append(extras_vec, extra_name_translation(pextra));
      }
    } extra_type_by_cause_iterate_end;
    if (strvec_size(extras_vec) > 0) {
      strvec_to_and_list(extras_vec, &extras_and);
      /* TRANS: %s is list of extra types separated by ',' and 'and' */
      cat_snprintf(buf, bufsz, _("* Can build %s on tiles.\n"),
                   astr_str(&extras_and));
      strvec_clear(extras_vec);
    }

    if (effect_cumulative_max(EFT_MINING_POSSIBLE, &for_utype) > 0) {
      extra_type_by_cause_iterate(EC_MINE, pextra) {
        if (help_is_extra_buildable(pextra, utype)) {
          strvec_append(extras_vec, extra_name_translation(pextra));
        }
      } extra_type_by_cause_iterate_end;

      if (strvec_size(extras_vec) > 0) {
        strvec_to_and_list(extras_vec, &extras_and);
        cat_snprintf(buf, bufsz, _("* Can build %s on tiles.\n"),
                     astr_str(&extras_and));
        strvec_clear(extras_vec);
      }
    }
    if (effect_cumulative_max(EFT_MINING_TF_POSSIBLE, &for_utype) > 0) {
      CATLSTR(buf, bufsz, _("* Can convert terrain to another type by "
                            "mining.\n"));
    }

    if (effect_cumulative_max(EFT_IRRIG_POSSIBLE, &for_utype) > 0) {
      extra_type_by_cause_iterate(EC_IRRIGATION, pextra) {
        if (help_is_extra_buildable(pextra, utype)) {
          strvec_append(extras_vec, extra_name_translation(pextra));
        }
      } extra_type_by_cause_iterate_end;

      if (strvec_size(extras_vec) > 0) {
        strvec_to_and_list(extras_vec, &extras_and);
        cat_snprintf(buf, bufsz, _("* Can build %s on tiles.\n"),
                     astr_str(&extras_and));
        strvec_clear(extras_vec);
      }
    }
    if (effect_cumulative_max(EFT_IRRIG_TF_POSSIBLE, &for_utype) > 0) {
      CATLSTR(buf, bufsz, _("* Can convert terrain to another type by "
                            "irrigation.\n"));
    }
    if (effect_cumulative_max(EFT_TRANSFORM_POSSIBLE, &for_utype) > 0) {
      CATLSTR(buf, bufsz, _("* Can transform terrain to another type.\n"));
    }

    extra_type_by_cause_iterate(EC_BASE, pextra) {
      if (help_is_extra_buildable(pextra, utype)) {
        strvec_append(extras_vec, extra_name_translation(pextra));
      }
    } extra_type_by_cause_iterate_end;

    if (strvec_size(extras_vec) > 0) {
      strvec_to_and_list(extras_vec, &extras_and);
      cat_snprintf(buf, bufsz, _("* Can build %s on tiles.\n"),
                   astr_str(&extras_and));
      strvec_clear(extras_vec);
    }

    /* Pollution, fallout. */
    extra_type_by_rmcause_iterate(ERM_CLEANPOLLUTION, pextra) {
      if (help_is_extra_cleanable(pextra, utype)) {
        strvec_append(extras_vec, extra_name_translation(pextra));
     }
    } extra_type_by_rmcause_iterate_end;

    if (strvec_size(extras_vec) > 0) {
      strvec_to_and_list(extras_vec, &extras_and);
      cat_snprintf(buf, bufsz, _("* Can clean %s from tiles.\n"),
                   astr_str(&extras_and));
      strvec_clear(extras_vec);
    }
    
    extra_type_by_rmcause_iterate(ERM_CLEANFALLOUT, pextra) {
      if (help_is_extra_cleanable(pextra, utype)) {
        strvec_append(extras_vec, extra_name_translation(pextra));
      }
    } extra_type_by_rmcause_iterate_end;

    if (strvec_size(extras_vec) > 0) {
      strvec_to_and_list(extras_vec, &extras_and);
      cat_snprintf(buf, bufsz, _("* Can clean %s from tiles.\n"),
                   astr_str(&extras_and));
      strvec_clear(extras_vec);
    }

    strvec_destroy(extras_vec);
  }

  if (utype_has_flag(utype, UTYF_SPY)) {
    CATLSTR(buf, bufsz, _("* Performs better diplomatic actions.\n"));
  }
  if (utype_has_flag(utype, UTYF_DIPLOMAT)
      || utype_has_flag(utype, UTYF_SUPERSPY)) {
    CATLSTR(buf, bufsz, _("* Defends cities against diplomatic actions.\n"));
  }
  if (utype_has_flag(utype, UTYF_SUPERSPY)) {
    CATLSTR(buf, bufsz, _("* Will never lose a diplomat-versus-diplomat fight.\n"));
  }
  if (utype_has_flag(utype, UTYF_SPY)
      && utype_has_flag(utype, UTYF_SUPERSPY)) {
    CATLSTR(buf, bufsz, _("* Will always survive a spy mission.\n"));
  }
  if (utype_has_flag(utype, UTYF_PARTIAL_INVIS)) {
    CATLSTR(buf, bufsz,
            _("* Is invisible except when next to an enemy unit or city.\n"));
  }
  if (utype_has_flag(utype, UTYF_ONLY_NATIVE_ATTACK)) {
    CATLSTR(buf, bufsz,
            _("* Can only attack units on native tiles.\n"));
  }
  /* Must use flag to distinguish from UCF_ATT_FROM_NON_NATIVE text. */
  if (utype_has_flag(utype, UTYF_MARINES)) {
    CATLSTR(buf, bufsz,
            _("* Can launch attack from non-native tiles.\n"));
  }
  if (game.info.slow_invasions
      && utype_has_flag(utype, UTYF_BEACH_LANDER)) {
    /* BeachLander only matters when slow_invasions are enabled. */
    CATLSTR(buf, bufsz,
            _("* Won't lose all movement when moving from non-native "
              "terrain to native terrain.\n"));
  }
  if (!uclass_has_flag(utype_class(utype), UCF_MISSILE)
      && utype_has_flag(utype, UTYF_ONEATTACK)) {
    CATLSTR(buf, bufsz,
	    _("* Making an attack ends this unit's turn.\n"));
  }
  if (utype_has_flag(utype, UTYF_CITYBUSTER)) {
    CATLSTR(buf, bufsz,
	    _("* Gets double firepower when attacking cities.\n"));
  }
  if (utype_has_flag(utype, UTYF_IGTER)) {
    cat_snprintf(buf, bufsz,
                 /* TRANS: "MP" = movement points. %s may have a 
                  * fractional part. */
                 _("* Ignores terrain effects (moving costs at most %s MP "
                   "per tile).\n"),
                 move_points_text(terrain_control.igter_cost, TRUE));
  }
  if (utype_has_flag(utype, UTYF_NOZOC)) {
    CATLSTR(buf, bufsz, _("* Never imposes a zone of control.\n"));
  } else {
    CATLSTR(buf, bufsz, _("* May impose a zone of control on its adjacent "
                          "tiles.\n"));
  }
  if (utype_has_flag(utype, UTYF_IGZOC)) {
    CATLSTR(buf, bufsz, _("* Not subject to zones of control imposed "
                          "by other units.\n"));
  }
  if (utype_has_flag(utype, UTYF_CIVILIAN)) {
    CATLSTR(buf, bufsz,
            _("* A non-military unit:\n"));
    CATLSTR(buf, bufsz,
            _("  * Cannot attack.\n"));
    CATLSTR(buf, bufsz,
            _("  * Doesn't impose martial law.\n"));
    CATLSTR(buf, bufsz,
            _("  * Can enter foreign territory regardless of peace treaty.\n"));
    CATLSTR(buf, bufsz,
            _("  * Doesn't prevent enemy cities from working the tile it's on.\n"));
  }
  if (utype_has_flag(utype, UTYF_FIELDUNIT)) {
    CATLSTR(buf, bufsz,
            _("* A field unit: one unhappiness applies even when non-aggressive.\n"));
  }
  if (utype_has_flag(utype, UTYF_SHIELD2GOLD)) {
    /* FIXME: the conversion shield => gold is activated if
     *        EFT_SHIELD2GOLD_FACTOR is not equal null; how to determine
     *        possible sources? */
    CATLSTR(buf, bufsz,
            _("* Under certain conditions the shield upkeep of this unit can "
              "be converted to gold upkeep.\n"));
  }

  unit_class_iterate(target) {
    if (uclass_has_flag(target, UCF_UNREACHABLE)
        && BV_ISSET(utype->targets, uclass_index(target))) {
      cat_snprintf(buf, bufsz,
                   _("* Can attack against %s units, which are usually not "
                     "reachable.\n"),
                   uclass_name_translation(target));
    }
  } unit_class_iterate_end;
  if (utype_fuel(utype)) {
    const char *types[utype_count()];
    int i = 0;

    unit_type_iterate(transport) {
      if (can_unit_type_transport(transport, utype_class(utype))) {
        types[i++] = utype_name_translation(transport);
      }
    } unit_type_iterate_end;

    if (0 == i) {
     cat_snprintf(buf, bufsz,
                   PL_("* Unit has to be in a city or a base"
                       " after %d turn.\n",
                       "* Unit has to be in a city or a base"
                       " after %d turns.\n",
                       utype_fuel(utype)),
                  utype_fuel(utype));
    } else {
      struct astring list = ASTRING_INIT;

      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is a list of unit types separated by "or" */
                   PL_("* Unit has to be in a city, a base, or on a %s"
                       " after %d turn.\n",
                       "* Unit has to be in a city, a base, or on a %s"
                       " after %d turns.\n",
                       utype_fuel(utype)),
                   astr_build_or_list(&list, types, i), utype_fuel(utype));
      astr_free(&list);
    }
  }
  action_iterate(act) {
    if (utype_can_do_action(utype, act)) {
      switch (act) {
      case ACTION_HELP_WONDER:
        cat_snprintf(buf, bufsz,
                     /* TRANS: the first %s is the ruleset defined ui
                      * name of the "Help Wonder" action, the next %s is
                      * the name of its target kind ("individual cities")
                      * and the %d is the number of shields the unit can
                      * contribute. */
                     _("* Can do the action \'%s\' to some %s"
                       " (adds %d production).\n"),
                     /* The action may have a ruleset defined ui name. */
                     action_get_ui_name(act),
                     /* Keep the style consistent with the help for the
                      * other actions. */
                     _(action_target_kind_name(
                         action_get_target_kind(act))),
                     /* The custom information. */
                     utype_build_shield_cost(utype));
        break;
      case ACTION_FOUND_CITY:
        cat_snprintf(buf, bufsz,
                     /* TRANS: the first %s is the ruleset defined ui
                      * name of the "Found City" action, the next %s is
                      * the name of its target kind ("tiles"), the %d
                      * is initial population and the third %s is if city
                      * founding is disabled in the current game. */
                     PL_("* Can do the action \'%s\' to some %s (initial"
                         " population %d).%s\n",
                         "* Can do the action \'%s\' to some %s (initial"
                         " population %d).%s\n",
                         utype->city_size),
                     /* The action may have a ruleset defined ui name. */
                     action_get_ui_name(act),
                     /* Keep the style consistent with the help for the
                      * other actions. */
                     _(action_target_kind_name(
                         action_get_target_kind(act))),
                     /* Custom information. */
                     utype->city_size,
                     game.scenario.prevent_new_cities ?
                       _(" (Disabled in the current game)") :
                       "");
        break;
      case ACTION_JOIN_CITY:
        cat_snprintf(buf, bufsz,
                     /* TRANS: the first %s is the ruleset defined ui
                      * name of the "Join City" action, the next %s is
                      * the name of its target kind ("individual cities")
                      * the first %d is population and the last %d, the
                      * value the plural is about, is the population
                      * added. */
                     PL_("* Can do the action \'%s\' to some %s of no"
                         " more than size %d (adds %d population).\n",
                         "* Can do the action \'%s\' to some %s of no"
                         " more than size %d (adds %d population).\n",
                         utype_pop_value(utype)),
                     /* The action may have a ruleset defined ui name. */
                     action_get_ui_name(act),
                     /* Keep the style consistent with the help for the
                                     * other actions. */
                     _(action_target_kind_name(
                         action_get_target_kind(act))),
                     /* Custom information. */
                     game.info.add_to_size_limit - utype_pop_value(utype),
                     utype_pop_value(utype));
        break;
      case ACTION_BOMBARD:
        cat_snprintf(buf, bufsz,
                     _("* Can do the action \'%s\' (%d per turn)."
                       " These attacks will only damage (never kill)"
                       " defenders, but damage all"
                       " defenders on a tile, and have no risk for the"
                       " attacker.\n"),
                     /* The action may have a ruleset defined UI
                                       * name. */
                     action_get_ui_name(act),
                     utype->bombard_rate);
        break;
      case ACTION_PARADROP:
        cat_snprintf(buf, bufsz,
                     _("* Can do the action \'%s\' from a friendly city "
                       "or suitable base (range: %d tiles).\n"),
                     action_get_ui_name(act), utype->paratroopers_range);
        break;
      default:
        /* Generic action information. */
        cat_snprintf(buf, bufsz,
                     /* TRANS: the first %s is the action's ruleset
                      * defined ui name and the next %s is the name of
                      * its target kind. */
                     _("* Can do the action \'%s\' to some %s.\n"),
                     action_get_ui_name(act),
                     _(action_target_kind_name(
                         action_get_target_kind(act))));
        break;
      }
    }
  } action_iterate_end;
  action_iterate(act) {
    bool vulnerable;

    /* Not relevant */
    if (action_get_target_kind(act) != ATK_UNIT
        && action_get_target_kind(act) != ATK_UNITS
        && action_get_target_kind(act) != ATK_SELF) {
      continue;
    }

    /* All units are immune to this since its not enabled */
    if (action_enabler_list_size(action_enablers_for_action(act)) == 0) {
      continue;
    }

    /* Must be immune in all cases */
    vulnerable = FALSE;
    action_enabler_list_iterate(action_enablers_for_action(act), enabler) {
      if (requirement_fulfilled_by_unit_type(utype,
                                             &(enabler->target_reqs))) {
        vulnerable = TRUE;
        break;
      }
    } action_enabler_list_iterate_end;

    if (!vulnerable) {
      cat_snprintf(buf, bufsz,
                   _("* Doing the action \'%s\' to this unit"
                     " is impossible.\n"),
                   action_get_ui_name(act));
    }
  } action_iterate_end;
  if (!has_vet_levels) {
    /* Only mention this if the game generally does have veteran levels. */
    if (game.veteran->levels > 1) {
      CATLSTR(buf, bufsz, _("* Will never achieve veteran status.\n"));
    }
  } else {
    /* Not useful currently: */
#if 0
    /* Some units can never become veteran through combat in practice. */
    bool veteran_through_combat =
      !((utype->attack_strength == 0
         || uclass_has_flag(utype_class(utype), UCF_MISSILE))
        && utype->defense_strength == 0);
#endif
    /* FIXME: if we knew the raise chances on the client, we could be
     * more specific here about whether veteran status can be acquired
     * through combat/missions/work. Should also take into account
     * UTYF_NO_VETERAN when writing this text. (Gna patch #4794) */
    CATLSTR(buf, bufsz, _("* May acquire veteran status.\n"));
    if (utype_veteran_has_power_bonus(utype)) {
      if ((!utype_can_do_action(utype, ACTION_NUKE)
           && utype->attack_strength > 0)
          || utype->defense_strength > 0) {
        CATLSTR(buf, bufsz,
                _("  * Veterans have increased strength in combat.\n"));
      }
      /* SUPERSPY always wins/escapes */
      if ((utype_has_flag(utype, UTYF_DIPLOMAT)
           || utype_has_flag(utype, UTYF_SPY))
          && !utype_has_flag(utype, UTYF_SUPERSPY)) {
        CATLSTR(buf, bufsz,
                _("  * Veterans have improved chances in diplomatic "
                  "contests.\n"));
        if (utype_has_flag(utype, UTYF_SPY)) {
          CATLSTR(buf, bufsz,
                  _("  * Veterans are more likely to survive missions.\n"));
        }
      }
      if (utype_has_flag(utype, UTYF_SETTLERS)) {
        CATLSTR(buf, bufsz,
                _("  * Veterans work faster.\n"));
      }
    }
  }
  if (strlen(buf) > 0) {
    CATLSTR(buf, bufsz, "\n");
  }
  if (has_vet_levels && utype->veteran) {
    /* The case where the unit has only a single veteran level has already
     * been handled above, so keep quiet here if that happens */
    if (insert_veteran_help(buf, bufsz, utype->veteran,
            _("This type of unit has its own veteran levels:"), NULL)) {
      CATLSTR(buf, bufsz, "\n\n");
    }
  }
  if (NULL != utype->helptext) {
    strvec_iterate(utype->helptext, text) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(text));
    } strvec_iterate_end;
  }
  CATLSTR(buf, bufsz, user_text);
  return buf;
}

/****************************************************************************
  Append misc dynamic text for advance/technology.

  pplayer may be NULL.
****************************************************************************/
void helptext_advance(char *buf, size_t bufsz, struct player *pplayer,
                      const char *user_text, int i)
{
  struct astring astr = ASTRING_INIT;
  struct advance *vap = valid_advance_by_number(i);
  struct universal source = {
    .kind = VUT_ADVANCE,
    .value = {.advance = vap}
  };
  int flagid;

  fc_assert_ret(NULL != buf && 0 < bufsz && NULL != user_text);
  fc_strlcpy(buf, user_text, bufsz);

  if (NULL == vap) {
    log_error("Unknown tech %d.", i);
    return;
  }

  if (NULL != pplayer) {
    const struct research *presearch = research_get(pplayer);

    if (research_invention_state(presearch, i) != TECH_KNOWN) {
      if (research_invention_state(presearch, i) == TECH_PREREQS_KNOWN) {
        int bulbs = research_total_bulbs_required(presearch, i, FALSE);

        cat_snprintf(buf, bufsz,
                     PL_("Starting now, researching %s would need %d bulb.",
                         "Starting now, researching %s would need %d bulbs.",
                         bulbs),
                     advance_name_translation(vap), bulbs);
      } else if (research_invention_reachable(presearch, i)) {
        /* Split string into two to allow localization of two pluralizations. */
        char buf2[MAX_LEN_MSG];
        int bulbs = research_goal_bulbs_required(presearch, i);

        fc_snprintf(buf2, ARRAY_SIZE(buf2),
                    /* TRANS: appended to another sentence. Preserve the
                     * leading space. */
                    PL_(" The whole project will require %d bulb to complete.",
                        " The whole project will require %d bulbs to complete.",
                        bulbs),
                    bulbs);
        cat_snprintf(buf, bufsz,
                     /* TRANS: last %s is a sentence pluralized separately. */
                     PL_("To reach %s you need to obtain %d other"
                         " technology first.%s",
                         "To reach %s you need to obtain %d other"
                         " technologies first.%s",
                         research_goal_unknown_techs(presearch, i) - 1),
                     advance_name_translation(vap),
                     research_goal_unknown_techs(presearch, i) - 1, buf2);
      } else {
        CATLSTR(buf, bufsz,
                _("You cannot research this technology."));
      }
      if (!techs_have_fixed_costs()
          && research_invention_reachable(presearch, i)) {
        CATLSTR(buf, bufsz,
                _(" This number may vary depending on what "
                  "other players research.\n"));
      } else {
        CATLSTR(buf, bufsz, "\n");
      }
    }

    CATLSTR(buf, bufsz, "\n");
  }

  if (requirement_vector_size(&vap->research_reqs) > 0) {
    CATLSTR(buf, bufsz, _("Requirements to research:\n"));
    requirement_vector_iterate(&vap->research_reqs, preq) {
      (void) insert_requirement(buf, bufsz, pplayer, preq);
    } requirement_vector_iterate_end;
    CATLSTR(buf, bufsz, "\n");
  }

  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));

  {
    int j;
    
    for (j = 0; j < MAX_NUM_TECH_LIST; j++) {
      if (game.rgame.global_init_techs[j] == A_LAST) {
        break;
      } else if (game.rgame.global_init_techs[j] == i) {
        CATLSTR(buf, bufsz,
                _("* All players start the game with knowledge of this "
                  "technology.\n"));
        break;
      }
    }
  }

  /* Assume no-one will set the same tech in both global and nation
   * init_tech... */
  nations_iterate(pnation) {
    int j;

    /* Avoid mentioning nations not in current set. */
    if (!show_help_for_nation(pnation)) {
      continue;
    }
    for (j = 0; j < MAX_NUM_TECH_LIST; j++) {
      if (pnation->init_techs[j] == A_LAST) {
        break;
      } else if (pnation->init_techs[j] == i) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a nation plural */
                     _("* The %s start the game with knowledge of this "
                       "technology.\n"), nation_plural_translation(pnation));
        break;
      }
    }
  } nations_iterate_end;

  if (advance_has_flag(i, TF_BONUS_TECH)) {
    cat_snprintf(buf, bufsz,
		 _("* The first player to research %s gets"
		   " an immediate advance.\n"),
                 advance_name_translation(vap));
  }

  for (flagid = TECH_USER_1 ; flagid <= TECH_USER_LAST; flagid++) {
    if (advance_has_flag(i, flagid)) {
      const char *helptxt = tech_flag_helptxt(flagid);

      if (helptxt != NULL) {
        CATLSTR(buf, bufsz, Q_("?bullet:* "));
        CATLSTR(buf, bufsz, _(helptxt));
        CATLSTR(buf, bufsz, "\n");
      }
    }
  }

  /* FIXME: bases -- but there is no good way to find out which bases a tech
   * can enable currently, so we have to remain silent. */

  if (game.info.tech_upkeep_style != TECH_UPKEEP_NONE) {
    CATLSTR(buf, bufsz,
            _("* To preserve this technology for our nation some bulbs "
              "are needed each turn.\n"));
  }

  if (NULL != vap->helptext) {
    if (strlen(buf) > 0) {
      CATLSTR(buf, bufsz, "\n");
    }
    strvec_iterate(vap->helptext, text) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(text));
    } strvec_iterate_end;
  }

  astr_free(&astr);
}

/****************************************************************
  Append text for terrain.
*****************************************************************/
void helptext_terrain(char *buf, size_t bufsz, struct player *pplayer,
		      const char *user_text, struct terrain *pterrain)
{
  struct universal source = {
    .kind = VUT_TERRAIN,
    .value = {.terrain = pterrain}
  };
  int flagid;

  fc_assert_ret(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  if (!pterrain) {
    log_error("Unknown terrain!");
    return;
  }

  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));
  if (terrain_has_flag(pterrain, TER_NO_CITIES)) {
    CATLSTR(buf, bufsz,
	    _("* You cannot build cities on this terrain."));
    CATLSTR(buf, bufsz, "\n");
  }
  if (pterrain->road_time == 0) {
    /* Can't build roads; only mention if ruleset has buildable roads */
    extra_type_by_cause_iterate(EC_ROAD, pextra) {
      if (pextra->buildable) {
        CATLSTR(buf, bufsz,
                _("* Paths cannot be built on this terrain."));
        CATLSTR(buf, bufsz, "\n");
        break;
      }
    } extra_type_by_cause_iterate_end;
  }
  if (pterrain->base_time == 0) {
    /* Can't build bases; only mention if ruleset has buildable bases */
    extra_type_by_cause_iterate(EC_BASE, pextra) {
      if (pextra->buildable) {
        CATLSTR(buf, bufsz,
                _("* Bases cannot be built on this terrain."));
        CATLSTR(buf, bufsz, "\n");
        break;
      }
    } extra_type_by_cause_iterate_end;
  }
  if (terrain_has_flag(pterrain, TER_UNSAFE_COAST)
      && terrain_type_terrain_class(pterrain) != TC_OCEAN) {
    CATLSTR(buf, bufsz,
	    _("* The coastline of this terrain is unsafe."));
    CATLSTR(buf, bufsz, "\n");
  }
  {
    const char *classes[uclass_count()];
    int i = 0;

    unit_class_iterate(uclass) {
      if (is_native_to_class(uclass, pterrain, NULL)) {
        classes[i++] = uclass_name_translation(uclass);
      }
    } unit_class_iterate_end;

    if (0 < i) {
      struct astring list = ASTRING_INIT;

      /* TRANS: %s is a list of unit classes separated by "and". */
      cat_snprintf(buf, bufsz, _("* Can be traveled by %s units.\n"),
                   astr_build_and_list(&list, classes, i));
      astr_free(&list);
    }
  }
  if (terrain_has_flag(pterrain, TER_NO_ZOC)) {
    CATLSTR(buf, bufsz,
            _("* Units on this terrain neither impose zones of control "
              "nor are restricted by them.\n"));
  } else {
    CATLSTR(buf, bufsz,
            _("* Units on this terrain may impose a zone of control, or "
              "be restricted by one.\n"));
  }
  if (terrain_has_flag(pterrain, TER_NO_FORTIFY)) {
    CATLSTR(buf, bufsz,
            _("* No units can fortify on this terrain.\n"));
  } else {
    CATLSTR(buf, bufsz,
            _("* Units able to fortify may do so on this terrain.\n"));
  }
  for (flagid = TER_USER_1 ; flagid <= TER_USER_LAST; flagid++) {
    if (terrain_has_flag(pterrain, flagid)) {
      const char *helptxt = terrain_flag_helptxt(flagid);

      if (helptxt != NULL) {
        CATLSTR(buf, bufsz, Q_("?bullet:* "));
        CATLSTR(buf, bufsz, _(helptxt));
        CATLSTR(buf, bufsz, "\n");
      }
    }
  }

  if (NULL != pterrain->helptext) {
    if (buf[0] != '\0') {
      CATLSTR(buf, bufsz, "\n");
    }
    strvec_iterate(pterrain->helptext, text) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(text));
    } strvec_iterate_end;
  }
  if (user_text && user_text[0] != '\0') {
    CATLSTR(buf, bufsz, "\n\n");
    CATLSTR(buf, bufsz, user_text);
  }
}

/****************************************************************************
  Return a textual representation of the F/P/T bonus a road provides to a
  terrain if supplied, or the terrain-independent bonus if pterrain==NULL.
  e.g. "0/0/+1", "0/+50%/0", or for a complex road "+2/+1+50%/0".
  Returns a pointer to a static string, so caller should not free
  (or NULL if there is no effect at all).
****************************************************************************/
const char *helptext_road_bonus_str(const struct terrain *pterrain,
                                    const struct road_type *proad)
{
  static char str[64];
  str[0] = '\0';
  bool has_effect = FALSE;
  output_type_iterate(o) {
    switch (o) {
    case O_FOOD:
    case O_SHIELD:
    case O_TRADE:
      {
        int bonus = proad->tile_bonus[o];
        int incr = proad->tile_incr_const[o];
        if (pterrain) {
          incr +=
            proad->tile_incr[o] * pterrain->road_output_incr_pct[o] / 100;
        }
        if (str[0] != '\0') {
          CATLSTR(str, sizeof(str), "/");
        }
        if (incr == 0 && bonus == 0) {
          cat_snprintf(str, sizeof(str), "%d", incr);
        } else {
          has_effect = TRUE;
          if (incr != 0) {
            cat_snprintf(str, sizeof(str), "%+d", incr);
          }
          if (bonus != 0) {
            cat_snprintf(str, sizeof(str), "%+d%%", bonus);
          }
        }
      }
      break;
    default:
      /* FIXME: there's nothing actually stopping roads having gold, etc
       * bonuses */
      fc_assert(proad->tile_incr_const[o] == 0
                && proad->tile_incr[o] == 0
                && proad->tile_bonus[o] == 0);
      break;
    }
  } output_type_iterate_end;

  return has_effect ? str : NULL;
}

/****************************************************************************
  Append misc dynamic text for extras.
  Assumes build time and conflicts are handled in the GUI front-end.

  pplayer may be NULL.
****************************************************************************/
void helptext_extra(char *buf, size_t bufsz, struct player *pplayer,
                    const char *user_text, struct extra_type *pextra)
{
  struct base_type *pbase;
  struct road_type *proad;
  struct universal source = {
    .kind = VUT_EXTRA,
    .value = {.extra = pextra}
  };

  fc_assert_ret(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  if (!pextra) {
    log_error("Unknown extra!");
    return;
  }

  if (is_extra_caused_by(pextra, EC_BASE)) {
    pbase = pextra->data.base;
  } else {
    pbase = NULL;
  }

  if (is_extra_caused_by(pextra, EC_ROAD)) {
    proad = pextra->data.road;
  } else {
    proad = NULL;
  }

  if (pextra->helptext != NULL) {
    strvec_iterate(pextra->helptext, text) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(text));
    } strvec_iterate_end;
  }

  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));

  if (is_extra_caused_by(pextra, EC_POLLUTION)) {
    CATLSTR(buf, bufsz,
            _("* May randomly appear around polluting city.\n"));
  }

  if (is_extra_caused_by(pextra, EC_FALLOUT)) {
    CATLSTR(buf, bufsz,
            _("* May randomly appear around nuclear blast.\n"));
  }

  if (is_extra_caused_by(pextra, EC_HUT)
      || (proad != NULL && road_has_flag(proad, RF_RIVER))) {
    CATLSTR(buf, bufsz,
            _("* Placed by mapgenerator.\n"));
  }

  if (is_extra_caused_by(pextra, EC_APPEARANCE)) {
    CATLSTR(buf, bufsz,
            _(" * May appear spontaneously.\n"));
  }

  /* XXX Non-zero requirement vector is not a good test of whether
   * insert_requirement() will give any output. */
  if (requirement_vector_size(&pextra->reqs) > 0) {
    if (pextra->buildable && is_extra_caused_by_worker_action(pextra)) {
      CATLSTR(buf, bufsz, _("Requirements to build:\n"));
    }
    requirement_vector_iterate(&pextra->reqs, preq) {
      (void) insert_requirement(buf, bufsz, pplayer, preq);
    } requirement_vector_iterate_end;
    CATLSTR(buf, bufsz, "\n");
  }

  {
    const char *classes[uclass_count()];
    int i = 0;

    unit_class_iterate(uclass) {
      if (is_native_extra_to_uclass(pextra, uclass)) {
        classes[i++] = uclass_name_translation(uclass);
      }
    } unit_class_iterate_end;

    if (0 < i) {
      struct astring list = ASTRING_INIT;

      if (proad != NULL) {
        /* TRANS: %s is a list of unit classes separated by "and". */
        cat_snprintf(buf, bufsz, _("* Can be traveled by %s units.\n"),
                     astr_build_and_list(&list, classes, i));
      } else {
        /* TRANS: %s is a list of unit classes separated by "and". */
        cat_snprintf(buf, bufsz, _("* Native to %s units.\n"),
                     astr_build_and_list(&list, classes, i));
      }
      astr_free(&list);

      if (extra_has_flag(pextra, EF_NATIVE_TILE)) {
        CATLSTR(buf, bufsz,
                _("  * Such units can move onto this tile even if it would "
                  "not normally be suitable terrain.\n"));
      }
      if (pbase != NULL) {
        if (base_has_flag(pbase, BF_NOT_AGGRESSIVE)) {
          /* "3 tiles" is hardcoded in is_friendly_city_near() */
          CATLSTR(buf, bufsz,
                  _("  * Such units situated here are not considered aggressive "
                    "if this tile is within 3 tiles of a friendly city.\n"));
        }
        if (territory_claiming_base(pbase)) {
          CATLSTR(buf, bufsz,
                  _("  * Can be captured by such units if at war with the "
                    "nation that currently owns it.\n"));
        }
        if (base_has_flag(pbase, BF_DIPLOMAT_DEFENSE)) {
          CATLSTR(buf, bufsz,
                  /* xgettext:no-c-format */
                  _("  * Diplomatic units get a 25% defense bonus in "
                    "diplomatic fights.\n"));
        }
      }
      if (pextra->defense_bonus) {
        cat_snprintf(buf, bufsz,
                     _("  * Such units get a %d%% defense bonus on this "
                       "tile.\n"),
                     pextra->defense_bonus);
      }
    }
  }

  if (proad != NULL && road_provides_move_bonus(proad)) {
    if (proad->move_cost == 0) {
      CATLSTR(buf, bufsz, _("* Allows infinite movement.\n"));
    } else {
      cat_snprintf(buf, bufsz,
                   /* TRANS: "MP" = movement points. Second %s may have a
                    * fractional part. */
                   _("* Movement cost along %s is %s MP.\n"),
                   road_name_translation(proad),
                   move_points_text(proad->move_cost, TRUE));
    }
  }

  if (is_extra_removed_by(pextra, ERM_PILLAGE)) {
    CATLSTR(buf, bufsz,
            _("* Can be pillaged by units.\n"));
  }
  if (is_extra_removed_by(pextra, ERM_CLEANPOLLUTION) || is_extra_removed_by(pextra, ERM_CLEANFALLOUT)) {
    CATLSTR(buf, bufsz,
            _("* Can be cleaned by units.\n"));
  }
  if (pbase != NULL) {
    if (game.info.killstack
        && base_has_flag(pbase, BF_NO_STACK_DEATH)) {
      CATLSTR(buf, bufsz,
              _("* Defeat of one unit does not cause death of all other units "
                "on this tile.\n"));
    }
    if (base_has_flag(pbase, BF_PARADROP_FROM)) {
      CATLSTR(buf, bufsz,
              _("* Units can paradrop from this tile.\n"));
    }
    if (territory_claiming_base(pbase)) {
      CATLSTR(buf, bufsz,
              _("* Extends national borders of the building nation.\n"));
    }
    if (pbase->vision_main_sq >= 0) {
      CATLSTR(buf, bufsz,
              _("* Grants permanent vision of an area around the tile to "
                "its owner.\n"));
    }
    if (pbase->vision_invis_sq >= 0) {
      CATLSTR(buf, bufsz,
              _("* Allows the owner to see normally invisible units in an "
                "area around the tile.\n"));
    }
  }

  /* Table of terrain-specific attributes, if needed */
  if (proad != NULL || pbase != NULL) {
    bool road, do_time, do_bonus;

    road = (proad != NULL);
    /* Terrain-dependent build time? */
    do_time = pextra->buildable && pextra->build_time == 0;
    if (road) {
      /* Terrain-dependent output bonus? */
      do_bonus = FALSE;
      output_type_iterate(o) {
        if (proad->tile_incr[o] > 0) {
          do_bonus = TRUE;
          fc_assert(o == O_FOOD || o == O_SHIELD || o == O_TRADE);
        }
      } output_type_iterate_end;
    } else {
      /* Bases don't have output bonuses */
      do_bonus = FALSE;
    }

    if (do_time || do_bonus) {
      if (do_time && do_bonus) {
        CATLSTR(buf, bufsz,
                _("\nTime to build and output bonus depends on terrain:\n\n"));
        CATLSTR(buf, bufsz,
                /* TRANS: Header for fixed-width road properties table.
                 * TRANS: Translators cannot change column widths :( */
                _("Terrain       Time     Bonus F/P/T\n"
                  "----------------------------------\n"));
      } else if (do_time) {
        CATLSTR(buf, bufsz,
                _("\nTime to build depends on terrain:\n\n"));
        CATLSTR(buf, bufsz,
                /* TRANS: Header for fixed-width extra properties table.
                 * TRANS: Translators cannot change column widths :( */
                _("Terrain       Time\n"
                  "------------------\n"));
      } else {
        fc_assert(do_bonus);
        CATLSTR(buf, bufsz,
                /* TRANS: Header for fixed-width road properties table.
                 * TRANS: Translators cannot change column widths :( */
                _("\nYields an output bonus with some terrains:\n\n"));
        CATLSTR(buf, bufsz,
                _("Terrain       Bonus F/P/T\n"
                  "-------------------------\n"));;
      }
      terrain_type_iterate(t) {
        int time = road ? terrain_extra_build_time(t, ACTIVITY_GEN_ROAD, pextra)
                          : terrain_extra_build_time(t, ACTIVITY_BASE, pextra);
        const char *bonus_text
          = road ? helptext_road_bonus_str(t, proad) : NULL;
        if (time > 0 || bonus_text) {
          const char *terrain = terrain_name_translation(t);
          cat_snprintf(buf, bufsz,
                       "%s%*s ", terrain,
                       MAX(0, 12 - (int)get_internal_string_length(terrain)),
                       "");
          if (do_time) {
            if (time > 0) {
              cat_snprintf(buf, bufsz, "%3d      ", time);
            } else {
              CATLSTR(buf, bufsz, "  -      ");
            }
          }
          if (do_bonus) {
            fc_assert(proad != NULL);
            cat_snprintf(buf, bufsz, " %s", bonus_text ? bonus_text : "-");
          }
          CATLSTR(buf, bufsz, "\n");
        }
      } terrain_type_iterate_end;
    } /* else rely on client-specific display */
  }

  if (user_text && user_text[0] != '\0') {
    CATLSTR(buf, bufsz, "\n\n");
    CATLSTR(buf, bufsz, user_text);
  }
}

/****************************************************************************
  Append misc dynamic text for specialists.
  Assumes effects are described in the help text.

  pplayer may be NULL.
****************************************************************************/
void helptext_specialist(char *buf, size_t bufsz, struct player *pplayer,
                         const char *user_text, struct specialist *pspec)
{
  bool reqs = FALSE;

  fc_assert_ret(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  if (NULL != pspec->helptext) {
    strvec_iterate(pspec->helptext, text) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(text));
    } strvec_iterate_end;
  }

  /* Requirements for this specialist. */
  requirement_vector_iterate(&pspec->reqs, preq) {
    if (insert_requirement(buf, bufsz, pplayer, preq)) {
      reqs = TRUE;
    }
  } requirement_vector_iterate_end;
  if (reqs) {
    fc_strlcat(buf, "\n", bufsz);
  }

  CATLSTR(buf, bufsz, user_text);
}

/****************************************************************
  Append text for government.

  pplayer may be NULL.

  TODO: Generalize the effects code for use elsewhere. Add
  other requirements.
*****************************************************************/
void helptext_government(char *buf, size_t bufsz, struct player *pplayer,
                         const char *user_text, struct government *gov)
{
  bool reqs = FALSE;
  struct universal source = {
    .kind = VUT_GOVERNMENT,
    .value = {.govern = gov}
  };

  fc_assert_ret(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  if (NULL != gov->helptext) {
    strvec_iterate(gov->helptext, text) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(text));
    } strvec_iterate_end;
  }

  /* Add requirement text for government itself */
  requirement_vector_iterate(&gov->reqs, preq) {
    if (insert_requirement(buf, bufsz, pplayer, preq)) {
      reqs = TRUE;
    }
  } requirement_vector_iterate_end;
  if (reqs) {
    fc_strlcat(buf, "\n", bufsz);
  }

  /* Effects */
  CATLSTR(buf, bufsz, _("Features:\n"));
  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));
  effect_list_iterate(get_req_source_effects(&source), peffect) {
    Output_type_id output_type = O_LAST;
    struct unit_class *unitclass = NULL;
    struct unit_type *unittype = NULL;
    enum unit_type_flag_id unitflag = unit_type_flag_id_invalid();
    struct strvec *outputs = strvec_new();
    struct astring outputs_or = ASTRING_INIT;
    struct astring outputs_and = ASTRING_INIT;
    bool extra_reqs = FALSE;
    bool world_value_valid = TRUE;

    /* Grab output type, if there is one */
    requirement_vector_iterate(&peffect->reqs, preq) {
      /* FIXME: perhaps we should treat any effect with negated requirements
       * as too complex for us to explain here? */
      if (!preq->present) {
        continue;
      }
      switch (preq->source.kind) {
       case VUT_OTYPE:
         /* We should never have multiple outputtype requirements
          * in one list in the first place (it simply makes no sense,
          * output cannot be of multiple types)
          * Ruleset loading code should check against that. */
         fc_assert(output_type == O_LAST);
         output_type = preq->source.value.outputtype;
         strvec_append(outputs, get_output_name(output_type));
         break;
       case VUT_UCLASS:
         fc_assert(unitclass == NULL);
         unitclass = preq->source.value.uclass;
         /* FIXME: can't easily get world bonus for unit class */
         world_value_valid = FALSE;
         break;
       case VUT_UTYPE:
         fc_assert(unittype == NULL);
         unittype = preq->source.value.utype;
         break;
       case VUT_UTFLAG:
         if (!unit_type_flag_id_is_valid(unitflag)) {
           unitflag = preq->source.value.unitflag;
           /* FIXME: can't easily get world bonus for unit type flag */
           world_value_valid = FALSE;
         } else {
           /* Already have a unit flag requirement. More than one is too
            * complex for us to explain, so say nothing. */
           /* FIXME: we could handle this */
           extra_reqs = TRUE;
         }
         break;
       case VUT_GOVERNMENT:
         /* This is government we are generating helptext for.
          * ...or if not, it's ruleset bug that should never make it
          * this far. Fix ruleset loading code. */
         fc_assert(preq->source.value.govern == gov);
         break;
       default:
         extra_reqs = TRUE;
         world_value_valid = FALSE;
         break;
      };
    } requirement_vector_iterate_end;

    if (!extra_reqs) {
      /* Only list effects that don't have extra requirements too complex
       * for us to handle.
       * Anything more complicated will have to be documented by hand by the
       * ruleset author. */

      /* Guard condition for simple player-wide effects descriptions.
       * (FIXME: in many cases, e.g. EFT_MAKE_CONTENT, additional requirements
       * like unittype will be ignored for gameplay, but will affect our
       * help here.) */
      const bool playerwide
        = world_value_valid && !unittype && (output_type == O_LAST);
      /* In some cases we give absolute values (world bonus + gov bonus).
       * We assume the fact that there's an effect with a gov requirement
       * is sufficient reason to list it in that gov's help.
       * Guard accesses to these with 'playerwide' or 'world_value_valid'. */
      int world_value = -999, net_value = -999;
      if (world_value_valid) {
        /* Get government-independent world value of effect if the extra
         * requirements were simple enough. */
        struct output_type *potype =
          output_type != O_LAST ? get_output_type(output_type) : NULL;
        world_value = 
          get_target_bonus_effects(NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, unittype, potype, NULL, NULL,
                                   peffect->type);
        net_value = peffect->value + world_value;
      }

      if (output_type == O_LAST) {
        /* There was no outputtype requirement. Effect is active for all
         * output types. Generate lists for that. */
        bool harvested_only = TRUE; /* Consider only output types from fields */

        if (peffect->type == EFT_UPKEEP_FACTOR
            || peffect->type == EFT_UNIT_UPKEEP_FREE_PER_CITY
            || peffect->type == EFT_OUTPUT_BONUS
            || peffect->type == EFT_OUTPUT_BONUS_2) {
          /* Effect can use or require any kind of output */
          harvested_only = FALSE;
        }

        output_type_iterate(ot) {
          struct output_type *pot = get_output_type(ot);

          if (!harvested_only || pot->harvested) {
            strvec_append(outputs, _(pot->name));
          }
        } output_type_iterate_end;
      }

      if (0 == strvec_size(outputs)) {
         /* TRANS: Empty output type list, should never happen. */
        astr_set(&outputs_or, "%s", Q_("?outputlist: Nothing "));
        astr_set(&outputs_and, "%s", Q_("?outputlist: Nothing "));
      } else {
        strvec_to_or_list(outputs, &outputs_or);
        strvec_to_and_list(outputs, &outputs_and);
      }

      switch (peffect->type) {
      case EFT_UNHAPPY_FACTOR:
        if (playerwide) {
          /* FIXME: EFT_MAKE_CONTENT_MIL_PER would cancel this out. We assume
           * no-one will set both, so we don't bother handling it. */
          cat_snprintf(buf, bufsz,
                       PL_("* Military units away from home and field units"
                           " will each cause %d citizen to become unhappy.\n",
                           "* Military units away from home and field units"
                           " will each cause %d citizens to become unhappy.\n",
                           net_value),
                       net_value);
        } /* else too complicated or silly ruleset */
        break;
      case EFT_ENEMY_CITIZEN_UNHAPPY_PCT:
        if (playerwide && net_value != world_value) {
          if (world_value > 0) {
            if (net_value > 0) {
              cat_snprintf(buf, bufsz,
                           _("* Unhappiness from foreign citizens due to "
                             "war with their home state is %d%% the usual "
                             "value.\n"),
                           (net_value * 100) / world_value);
            } else {
              CATLSTR(buf, bufsz,
                      _("* No unhappiness from foreign citizens even when "
                        "at war with their home state.\n"));
            }
          } else {
            cat_snprintf(buf, bufsz,
                         /* TRANS: not pluralised as gettext doesn't support
                          * fractional numbers, which this might be */
                         _("* Each foreign citizen causes %.2g unhappiness "
                           "in their city while you are at war with their "
                           "home state.\n"),
                         (double)net_value / 100);
          }
        }
        break;
      case EFT_MAKE_CONTENT_MIL:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* Each of your cities will avoid %d unhappiness"
                           " caused by units.\n",
                           "* Each of your cities will avoid %d unhappiness"
                           " caused by units.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_MAKE_CONTENT:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* Each of your cities will avoid %d unhappiness,"
                           " not including that caused by aggression.\n",
                           "* Each of your cities will avoid %d unhappiness,"
                           " not including that caused by aggression.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_FORCE_CONTENT:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* Each of your cities will avoid %d unhappiness,"
                           " including that caused by aggression.\n",
                           "* Each of your cities will avoid %d unhappiness,"
                           " including that caused by aggression.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_UPKEEP_FACTOR:
        if (world_value_valid && !unittype) {
          if (net_value == 0) {
            if (output_type != O_LAST) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is the output type, like 'shield'
                            * or 'gold'. */
                           _("* You pay no %s upkeep for your units.\n"),
                           astr_str(&outputs_or));
            } else {
              CATLSTR(buf, bufsz,
                      _("* You pay no upkeep for your units.\n"));
            }
          } else if (net_value != world_value) {
            double ratio = (double)net_value / world_value;
            if (output_type != O_LAST) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is the output type, like 'shield'
                            * or 'gold'. */
                           _("* You pay %.2g times normal %s upkeep for your "
                             "units.\n"),
                           ratio, astr_str(&outputs_and));
            } else {
              cat_snprintf(buf, bufsz,
                           _("* You pay %.2g times normal upkeep for your "
                             "units.\n"),
                           ratio);
            }
          } /* else this effect somehow has no effect; keep quiet */
        } /* else there was some extra condition making it complicated */
        break;
      case EFT_UNIT_UPKEEP_FREE_PER_CITY:
        if (!unittype) {
          if (output_type != O_LAST) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is the output type, like 'shield' or
                          * 'gold'; pluralised in %d but there is currently
                          * no way to control the singular/plural name of the
                          * output type; sorry */
                         PL_("* Each of your cities will avoid paying %d %s"
                             " upkeep for your units.\n",
                             "* Each of your cities will avoid paying %d %s"
                             " upkeep for your units.\n", peffect->value),
                         peffect->value, astr_str(&outputs_and));
          } else {
            cat_snprintf(buf, bufsz,
                         /* TRANS: Amount is subtracted from upkeep cost
                          * for each upkeep type. */
                         PL_("* Each of your cities will avoid paying %d"
                             " upkeep for your units.\n",
                             "* Each of your cities will avoid paying %d"
                             " upkeep for your units.\n", peffect->value),
                         peffect->value);
          }
        } /* else too complicated */
        break;
      case EFT_CIVIL_WAR_CHANCE:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       _("* If you lose your capital,"
                         " the chance of civil war is %d%%.\n"),
                       net_value);
        }
        break;
      case EFT_EMPIRE_SIZE_BASE:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* You can have %d city before an "
                           "additional unhappy citizen appears in each city "
                           "due to civilization size.\n",
                           "* You can have up to %d cities before an "
                           "additional unhappy citizen appears in each city "
                           "due to civilization size.\n", net_value),
                       net_value);
        }
        break;
      case EFT_EMPIRE_SIZE_STEP:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* After the first unhappy citizen due to"
                           " civilization size, for each %d additional city"
                           " another unhappy citizen will appear.\n",
                           "* After the first unhappy citizen due to"
                           " civilization size, for each %d additional cities"
                           " another unhappy citizen will appear.\n",
                           net_value),
                       net_value);
        }
        break;
      case EFT_MAX_RATES:
        if (playerwide && game.info.changable_tax) {
          if (net_value < 100) {
            cat_snprintf(buf, bufsz,
                         _("* The maximum rate you can set for science,"
                            " gold, or luxuries is %d%%.\n"),
                         net_value);
          } else {
            CATLSTR(buf, bufsz,
                    _("* Has unlimited science/gold/luxuries rates.\n"));
          }
        }
        break;
      case EFT_MARTIAL_LAW_EACH:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* Your units may impose martial law."
                           " Each military unit inside a city will force %d"
                           " unhappy citizen to become content.\n",
                           "* Your units may impose martial law."
                           " Each military unit inside a city will force %d"
                           " unhappy citizens to become content.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_MARTIAL_LAW_MAX:
        if (playerwide && net_value < 100) {
          cat_snprintf(buf, bufsz,
                       PL_("* A maximum of %d unit in each city can enforce"
                           " martial law.\n",
                           "* A maximum of %d units in each city can enforce"
                           " martial law.\n",
                           net_value),
                       net_value);
        }
        break;
      case EFT_RAPTURE_GROW:
        if (playerwide && net_value > 0) {
          cat_snprintf(buf, bufsz,
                       _("* You may grow your cities by means of "
                         "celebrations."));
          if (game.info.celebratesize > 1) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: Preserve leading space. %d should always be
                          * 2 or greater. */
                         _(" (Cities below size %d cannot grow in this way.)"),
                         game.info.celebratesize);
          }
          cat_snprintf(buf, bufsz, "\n");
        }
        break;
      case EFT_REVOLUTION_UNHAPPINESS:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* If a city is in disorder for more than %d turn "
                           "in a row, government will fall into anarchy.\n",
                           "* If a city is in disorder for more than %d turns "
                           "in a row, government will fall into anarchy.\n",
                           net_value),
                       net_value);
        }
        break;
      case EFT_HAS_SENATE:
        if (playerwide && net_value > 0) {
          CATLSTR(buf, bufsz,
                  _("* Has a senate that may prevent declaration of war.\n"));
        }
        break;
      case EFT_INSPIRE_PARTISANS:
        if (playerwide && net_value > 0) {
          CATLSTR(buf, bufsz,
                  _("* Allows partisans when cities are taken by the "
                    "enemy.\n"));
        }
        break;
      case EFT_HAPPINESS_TO_GOLD:
        if (playerwide && net_value > 0) {
          CATLSTR(buf, bufsz,
                  _("* Buildings that normally confer bonuses against"
                    " unhappiness will instead give gold.\n"));
        }
        break;
      case EFT_FANATICS:
        if (playerwide && net_value > 0) {
          struct strvec *fanatics = strvec_new();
          struct astring fanaticstr = ASTRING_INIT;

          unit_type_iterate(putype) {
            if (utype_has_flag(putype, UTYF_FANATIC)) {
              strvec_append(fanatics, utype_name_translation(putype));
            }
          } unit_type_iterate_end;
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is list of unit types separated by 'or' */
                       _("* Pays no upkeep for %s.\n"),
                       strvec_to_or_list(fanatics, &fanaticstr));
          strvec_destroy(fanatics);
          astr_free(&fanaticstr);
        }
        break;
      case EFT_NO_UNHAPPY:
        if (playerwide && net_value > 0) {
          CATLSTR(buf, bufsz, _("* Has no unhappy citizens.\n"));
        }
        break;
      case EFT_VETERAN_BUILD:
        {
          int conditions = 0;
          if (unitclass) {
            conditions++;
          }
          if (unittype) {
            conditions++;
          }
          if (unit_type_flag_id_is_valid(unitflag)) {
            conditions++;
          }
          if (conditions > 1) {
            /* More than one requirement on units, too complicated for us
             * to describe. */
            break;
          }
          if (unitclass) {
            /* FIXME: account for multiple veteran levels, or negative
             * values. This might lie for complicated rulesets! */
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is a unit class */
                         Q_("?unitclass:* New %s units will be veteran.\n"),
                         uclass_name_translation(unitclass));
          } else if (unit_type_flag_id_is_valid(unitflag)) {
            /* FIXME: same problems as unitclass */
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is a (translatable) unit type flag */
                         Q_("?unitflag:* New %s units will be veteran.\n"),
                         unit_type_flag_id_translated_name(unitflag));
          } else if (unittype != NULL) {
            if (world_value_valid && net_value > 0) {
              /* Here we can be specific about veteran level, and get
               * net value correct. */
              int maxlvl = utype_veteran_system(unittype)->levels - 1;
              const struct veteran_level *vlevel =
                utype_veteran_level(unittype, MIN(net_value, maxlvl));
              cat_snprintf(buf, bufsz,
                           /* TRANS: "* New Partisan units will have the rank
                            * of elite." */
                           Q_("?unittype:* New %s units will have the rank "
                              "of %s.\n"),
                           utype_name_translation(unittype),
                           name_translation_get(&vlevel->name));
            } /* else complicated */
          } else {
            /* No extra criteria. */
            /* FIXME: same problems as above */
            cat_snprintf(buf, bufsz,
                         _("* New units will be veteran.\n"));
          }
        }
        break;
      case EFT_OUTPUT_PENALTY_TILE:
        if (world_value_valid) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is list of output types, with 'or';
                        * pluralised in %d but of course the output types
                        * can't be pluralised; sorry */
                       PL_("* Each worked tile that gives more than %d %s will"
                           " suffer a -1 penalty, unless the city working it"
                           " is celebrating.",
                           "* Each worked tile that gives more than %d %s will"
                           " suffer a -1 penalty, unless the city working it"
                           " is celebrating.", net_value),
                       net_value, astr_str(&outputs_or));
          if (game.info.celebratesize > 1) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: Preserve leading space. %d should always be
                          * 2 or greater. */
                         _(" (Cities below size %d will not celebrate.)"),
                         game.info.celebratesize);
          }
          cat_snprintf(buf, bufsz, "\n");
        }
        break;
      case EFT_OUTPUT_INC_TILE_CELEBRATE:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is list of output types, with 'or' */
                     PL_("* Each worked tile with at least 1 %s will yield"
                         " %d more of it while the city working it is"
                         " celebrating.",
                         "* Each worked tile with at least 1 %s will yield"
                         " %d more of it while the city working it is"
                         " celebrating.", peffect->value),
                     astr_str(&outputs_or), peffect->value);
        if (game.info.celebratesize > 1) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: Preserve leading space. %d should always be
                        * 2 or greater. */
                       _(" (Cities below size %d will not celebrate.)"),
                       game.info.celebratesize);
        }
        cat_snprintf(buf, bufsz, "\n");
        break;
      case EFT_OUTPUT_INC_TILE:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is list of output types, with 'or' */
                     PL_("* Each worked tile with at least 1 %s will yield"
                         " %d more of it.\n",
                         "* Each worked tile with at least 1 %s will yield"
                         " %d more of it.\n", peffect->value),
                     astr_str(&outputs_or), peffect->value);
        break;
      case EFT_OUTPUT_BONUS:
      case EFT_OUTPUT_BONUS_2:
        /* FIXME: makes most sense iff world_value == 0 */
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is list of output types, with 'and' */
                     _("* %s production is increased %d%%.\n"),
                     astr_str(&outputs_and), peffect->value);
        break;
      case EFT_OUTPUT_WASTE:
        if (world_value_valid) {
          if (net_value > 30) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is list of output types, with 'and' */
                         _("* %s production will suffer massive losses.\n"),
                         astr_str(&outputs_and));
          } else if (net_value >= 15) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is list of output types, with 'and' */
                         _("* %s production will suffer some losses.\n"),
                         astr_str(&outputs_and));
          } else if (net_value > 0) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is list of output types, with 'and' */
                         _("* %s production will suffer a small amount "
                           "of losses.\n"),
                         astr_str(&outputs_and));
          }
        }
        break;
      case EFT_HEALTH_PCT:
        if (playerwide) {
          if (peffect->value > 0) {
            CATLSTR(buf, bufsz, _("* Increases the chance of plague"
                                  " within your cities.\n"));
          } else if (peffect->value < 0) {
            CATLSTR(buf, bufsz, _("* Decreases the chance of plague"
                                  " within your cities.\n"));
          }
        }
        break;
      case EFT_OUTPUT_WASTE_BY_DISTANCE:
        if (world_value_valid) {
          if (net_value >= 3) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is list of output types, with 'and' */
                         _("* %s losses will increase quickly"
                           " with distance from capital.\n"),
                         astr_str(&outputs_and));
          } else if (net_value == 2) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is list of output types, with 'and' */
                         _("* %s losses will increase"
                           " with distance from capital.\n"),
                         astr_str(&outputs_and));
          } else if (net_value > 0) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is list of output types, with 'and' */
                         _("* %s losses will increase slowly"
                           " with distance from capital.\n"),
                         astr_str(&outputs_and));
          }
        }
        break;
      case EFT_MIGRATION_PCT:
        if (playerwide) {
          if (peffect->value > 0) {
            CATLSTR(buf, bufsz, _("* Increases the chance of migration"
                                  " into your cities.\n"));
          } else if (peffect->value < 0) {
            CATLSTR(buf, bufsz, _("* Decreases the chance of migration"
                                  " into your cities.\n"));
          }
        }
        break;
      case EFT_BORDER_VISION:
        if (game.info.borders == BORDERS_ENABLED
            && playerwide && net_value > 0) {
          CATLSTR(buf, bufsz, _("* All tiles inside your borders are"
                                " monitored.\n"));
        }
        break;
      default:
        break;
      };
    }

    strvec_destroy(outputs);
    astr_free(&outputs_or);
    astr_free(&outputs_and);

  } effect_list_iterate_end;

  unit_type_iterate(utype) {
    if (utype->need_government == gov) {
      cat_snprintf(buf, bufsz,
                   _("* Allows you to build %s.\n"),
                   utype_name_translation(utype));
    }
  } unit_type_iterate_end;

  /* Action immunity */
  action_iterate(act) {
    if (action_immune_government(gov, act)) {
      cat_snprintf(buf, bufsz,
                   _("* Makes it impossible to do the action \'%s\'"
                     " to your %s.\n"),
                   action_get_ui_name(act),
                   _(action_target_kind_name(action_get_target_kind(act))));
    }
  } action_iterate_end;

  if (user_text && user_text[0] != '\0') {
    cat_snprintf(buf, bufsz, "\n%s", user_text);
  }
}

/****************************************************************
  Returns pointer to static string with eg: "1 shield, 1 unhappy"
*****************************************************************/
char *helptext_unit_upkeep_str(struct unit_type *utype)
{
  static char buf[128];
  int any = 0;

  if (!utype) {
    log_error("Unknown unit!");
    return "";
  }


  buf[0] = '\0';
  output_type_iterate(o) {
    if (utype->upkeep[o] > 0) {
      /* TRANS: "2 Food" or ", 1 Shield" */
      cat_snprintf(buf, sizeof(buf), _("%s%d %s"),
	      (any > 0 ? Q_("?blistmore:, ") : ""), utype->upkeep[o],
	      get_output_name(o));
      any++;
    }
  } output_type_iterate_end;
  if (utype->happy_cost > 0) {
    /* TRANS: "2 Unhappy" or ", 1 Unhappy" */
    cat_snprintf(buf, sizeof(buf), _("%s%d Unhappy"),
	    (any > 0 ? Q_("?blistmore:, ") : ""), utype->happy_cost);
    any++;
  }

  if (any == 0) {
    /* strcpy(buf, _("None")); */
    fc_snprintf(buf, sizeof(buf), "%d", 0);
  }
  return buf;
}

/****************************************************************************
  Returns nation legend and characteristics
****************************************************************************/
void helptext_nation(char *buf, size_t bufsz, struct nation_type *pnation,
		     const char *user_text)
{
  struct universal source = {
    .kind = VUT_NATION,
    .value = {.nation = pnation}
  };

  fc_assert_ret(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  if (pnation->legend[0] != '\0') {
    /* Client side legend is stored already translated */
    sprintf(buf, "%s\n\n", pnation->legend);
  }
  
  cat_snprintf(buf, bufsz,
               _("Initial government is %s.\n"),
               government_name_translation(pnation->init_government));
  if (pnation->init_techs[0] != A_LAST) {
    const char *tech_names[MAX_NUM_TECH_LIST];
    int i;
    struct astring list = ASTRING_INIT;
    for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
      if (pnation->init_techs[i] == A_LAST) {
        break;
      }
      tech_names[i] =
        advance_name_translation(advance_by_number(pnation->init_techs[i]));
    }
    astr_build_and_list(&list, tech_names, i);
    if (game.rgame.global_init_techs[0] != A_LAST) {
      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is an and-separated list of techs */
                   _("Starts with knowledge of %s in addition to the standard "
                     "starting technologies.\n"), astr_str(&list));
    } else {
      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is an and-separated list of techs */
                   _("Starts with knowledge of %s.\n"), astr_str(&list));
    }
    astr_free(&list);
  }
  if (pnation->init_units[0]) {
    const struct unit_type *utypes[MAX_NUM_UNIT_LIST];
    int count[MAX_NUM_UNIT_LIST];
    int i, j, n = 0, total = 0;
    /* Count how many of each type there is. */
    for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
      if (!pnation->init_units[i]) {
        break;
      }
      for (j = 0; j < n; j++) {
        if (pnation->init_units[i] == utypes[j]) {
          count[j]++;
          total++;
          break;
        }
      }
      if (j == n) {
        utypes[n] = pnation->init_units[i];
        count[n] = 1;
        total++;
        n++;
      }
    }
    {
      /* Construct the list of unit types and counts. */
      struct astring utype_names[MAX_NUM_UNIT_LIST];
      struct astring list = ASTRING_INIT;
      for (i = 0; i < n; i++) {
        astr_init(&utype_names[i]);
        if (count[i] > 1) {
          /* TRANS: a unit type followed by a count. For instance,
           * "Fighter (2)" means two Fighters. Count is never 1. 
           * Used in a list. */
          astr_set(&utype_names[i], _("%s (%d)"),
                   utype_name_translation(utypes[i]), count[i]);
        } else {
          astr_set(&utype_names[i], "%s", utype_name_translation(utypes[i]));
        }
      }
      {
        const char *utype_name_strs[MAX_NUM_UNIT_LIST];
        for (i = 0; i < n; i++) {
          utype_name_strs[i] = astr_str(&utype_names[i]);
        }
        astr_build_and_list(&list, utype_name_strs, n);
      }
      for (i = 0; i < n; i++) {
        astr_free(&utype_names[i]);
      }
      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is an and-separated list of unit types
                    * possibly with counts. Plurality is in total number of
                    * units represented. */
                   PL_("Starts with the following additional unit: %s.\n",
                       "Starts with the following additional units: %s.\n",
                      total), astr_str(&list));
      astr_free(&list);
    }
  }
  if (pnation->init_buildings[0] != B_LAST) {
    const char *impr_names[MAX_NUM_BUILDING_LIST];
    int i;
    struct astring list = ASTRING_INIT;
    for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
      if (pnation->init_buildings[i] == B_LAST) {
        break;
      }
      impr_names[i] =
        improvement_name_translation(
          improvement_by_number(pnation->init_buildings[i]));
    }
    astr_build_and_list(&list, impr_names, i);
    if (game.rgame.global_init_buildings[0] != B_LAST) {
      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is an and-separated list of improvements */
                   _("First city will get %s for free in addition to the "
                     "standard improvements.\n"), astr_str(&list));
    } else {
      cat_snprintf(buf, bufsz,
                   /* TRANS: %s is an and-separated list of improvements */
                   _("First city will get %s for free.\n"), astr_str(&list));
    }
    astr_free(&list);
  }

  if (buf[0] != '\0') {
    CATLSTR(buf, bufsz, "\n");
  }
  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));

  if (user_text && user_text[0] != '\0') {
    if (buf[0] != '\0') {
      CATLSTR(buf, bufsz, "\n");
    }
    CATLSTR(buf, bufsz, user_text);
  }
}
