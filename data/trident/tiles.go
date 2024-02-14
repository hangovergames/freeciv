package images

type Tile struct {
	XTopLeft, YTopLeft, DX, DY int
	Tiles                      []TileSpec
}

type TileSpec struct {
	Row, Column int
	Tag         string
}

// Data from the [grid_main] section
var GridMain = Tile{
	XTopLeft: 0,
	YTopLeft: 0,
	DX:       30,
	DY:       30,
	Tiles: []TileSpec{

		// Grassland, and whether terrain to north, south, east, west
		// is more grassland:

		{Row: 0, Column: 2, Tag: "t.l0.grassland1"},
		{Row: 0, Column: 3, Tag: "t.l0.inaccessible1"},

		{Row: 0, Column: 1, Tag: "t.l1.grassland1"},
		{Row: 0, Column: 1, Tag: "t.l1.hills1"},
		{Row: 0, Column: 1, Tag: "t.l1.forest1"},
		{Row: 0, Column: 1, Tag: "t.l1.mountains1"},
		{Row: 0, Column: 1, Tag: "t.l1.desert1"},
		{Row: 0, Column: 1, Tag: "t.l1.jungle1"},
		{Row: 0, Column: 1, Tag: "t.l1.plains1"},
		{Row: 0, Column: 1, Tag: "t.l1.swamp1"},
		{Row: 0, Column: 1, Tag: "t.l1.tundra1"},

		{Row: 0, Column: 1, Tag: "t.l2.grassland1"},
		{Row: 0, Column: 1, Tag: "t.l2.hills1"},
		{Row: 0, Column: 1, Tag: "t.l2.forest1"},
		{Row: 0, Column: 1, Tag: "t.l2.mountains1"},
		{Row: 0, Column: 1, Tag: "t.l2.desert1"},
		{Row: 0, Column: 1, Tag: "t.l2.arctic1"},
		{Row: 0, Column: 1, Tag: "t.l2.jungle1"},
		{Row: 0, Column: 1, Tag: "t.l2.plains1"},
		{Row: 0, Column: 1, Tag: "t.l2.swamp1"},
		{Row: 0, Column: 1, Tag: "t.l2.tundra1"},


		// For hills, forest and mountains don't currently have a full set,
		// re-use values but provide for future expansion// current sets
		// effectively ignore N/S terrain.

		// Hills, and whether terrain to north, south, east, west
		// is more hills.

		{Row: 0, Column: 4, Tag: "t.l0.hills_n0e0s0w0"}, // not-hills E and W
		{Row: 0, Column: 4, Tag: "t.l0.hills_n0e0s1w0"},
		{Row: 0, Column: 4, Tag: "t.l0.hills_n1e0s0w0"},
		{Row: 0, Column: 4, Tag: "t.l0.hills_n1e0s1w0"},

		{Row: 0, Column: 5, Tag: "t.l0.hills_n0e1s0w0"}, // hills E
		{Row: 0, Column: 5, Tag: "t.l0.hills_n0e1s1w0"},
		{Row: 0, Column: 5, Tag: "t.l0.hills_n1e1s0w0"},
		{Row: 0, Column: 5, Tag: "t.l0.hills_n1e1s1w0"},

		{Row: 0, Column: 6, Tag: "t.l0.hills_n0e1s0w1"}, // hills E and W
		{Row: 0, Column: 6, Tag: "t.l0.hills_n0e1s1w1"},
		{Row: 0, Column: 6, Tag: "t.l0.hills_n1e1s0w1"},
		{Row: 0, Column: 6, Tag: "t.l0.hills_n1e1s1w1"},

		{Row: 0, Column: 7, Tag: "t.l0.hills_n0e0s0w1"}, // hills W
		{Row: 0, Column: 7, Tag: "t.l0.hills_n0e0s1w1"},
		{Row: 0, Column: 7, Tag: "t.l0.hills_n1e0s0w1"},
		{Row: 0, Column: 7, Tag: "t.l0.hills_n1e0s1w1"},

		// Forest, and whether terrain to north, south, east, west
		// is more forest.
		{Row: 0, Column: 8, Tag: "t.l0.forest_n0e0s0w0"}, // not-forest E and W
		{Row: 0, Column: 8, Tag: "t.l0.forest_n0e0s1w0"},
		{Row: 0, Column: 8, Tag: "t.l0.forest_n1e0s0w0"},
		{Row: 0, Column: 8, Tag: "t.l0.forest_n1e0s1w0"},

		{Row: 0, Column: 9, Tag: "t.l0.forest_n0e1s0w0"}, // forest E
		{Row: 0, Column: 9, Tag: "t.l0.forest_n0e1s1w0"},
		{Row: 0, Column: 9, Tag: "t.l0.forest_n1e1s0w0"},
		{Row: 0, Column: 9, Tag: "t.l0.forest_n1e1s1w0"},

		{Row: 0, Column: 10, Tag: "t.l0.forest_n0e1s0w1"}, // forest E and W
		{Row: 0, Column: 10, Tag: "t.l0.forest_n0e1s1w1"},
		{Row: 0, Column: 10, Tag: "t.l0.forest_n1e1s0w1"},
		{Row: 0, Column: 10, Tag: "t.l0.forest_n1e1s1w1"},

		{Row: 0, Column: 11, Tag: "t.l0.forest_n0e0s0w1"}, // forest W
		{Row: 0, Column: 11, Tag: "t.l0.forest_n0e0s1w1"},
		{Row: 0, Column: 11, Tag: "t.l0.forest_n1e0s0w1"},
		{Row: 0, Column: 11, Tag: "t.l0.forest_n1e0s1w1"},

		// Mountains, and whether terrain to north, south, east, west
		// is more mountains.

		{Row: 0, Column: 12, Tag: "t.l0.mountains_n0e0s0w0"}, // not-mountains E and W
		{Row: 0, Column: 12, Tag: "t.l0.mountains_n0e0s1w0"},
		{Row: 0, Column: 12, Tag: "t.l0.mountains_n1e0s0w0"},
		{Row: 0, Column: 12, Tag: "t.l0.mountains_n1e0s1w0"},

		{Row: 0, Column: 13, Tag: "t.l0.mountains_n0e1s0w0"}, // mountains E
		{Row: 0, Column: 13, Tag: "t.l0.mountains_n0e1s1w0"},
		{Row: 0, Column: 13, Tag: "t.l0.mountains_n1e1s0w0"},
		{Row: 0, Column: 13, Tag: "t.l0.mountains_n1e1s1w0"},

		{Row: 0, Column: 14, Tag: "t.l0.mountains_n0e1s0w1"}, // mountains E and W
		{Row: 0, Column: 14, Tag: "t.l0.mountains_n0e1s1w1"},
		{Row: 0, Column: 14, Tag: "t.l0.mountains_n1e1s0w1"},
		{Row: 0, Column: 14, Tag: "t.l0.mountains_n1e1s1w1"},

		{Row: 0, Column: 15, Tag: "t.l0.mountains_n0e0s0w1"}, // mountains W
		{Row: 0, Column: 15, Tag: "t.l0.mountains_n0e0s1w1"},
		{Row: 0, Column: 15, Tag: "t.l0.mountains_n1e0s0w1"},
		{Row: 0, Column: 15, Tag: "t.l0.mountains_n1e0s1w1"},

		// Desert, and whether terrain to north, south, east, west
		// is more desert:

		{Row: 1, Column: 0, Tag: "t.l0.desert_n1e1s1w1"},
		{Row: 1, Column: 1, Tag: "t.l0.desert_n0e1s1w1"},
		{Row: 1, Column: 2, Tag: "t.l0.desert_n1e0s1w1"},
		{Row: 1, Column: 3, Tag: "t.l0.desert_n0e0s1w1"},
		{Row: 1, Column: 4, Tag: "t.l0.desert_n1e1s0w1"},
		{Row: 1, Column: 5, Tag: "t.l0.desert_n0e1s0w1"},
		{Row: 1, Column: 6, Tag: "t.l0.desert_n1e0s0w1"},
		{Row: 1, Column: 7, Tag: "t.l0.desert_n0e0s0w1"},
		{Row: 1, Column: 8, Tag: "t.l0.desert_n1e1s1w0"},
		{Row: 1, Column: 9, Tag: "t.l0.desert_n0e1s1w0"},
		{Row: 1, Column: 10, Tag: "t.l0.desert_n1e0s1w0"},
		{Row: 1, Column: 11, Tag: "t.l0.desert_n0e0s1w0"},
		{Row: 1, Column: 12, Tag: "t.l0.desert_n1e1s0w0"},
		{Row: 1, Column: 13, Tag: "t.l0.desert_n0e1s0w0"},
		{Row: 1, Column: 14, Tag: "t.l0.desert_n1e0s0w0"},
		{Row: 1, Column: 15, Tag: "t.l0.desert_n0e0s0w0"},

		// Arctic, and whether terrain to north, south, east, west
		// is more arctic:

		{Row: 6, Column: 0, Tag: "t.l0.arctic_n1e1s1w1"},
		{Row: 6, Column: 1, Tag: "t.l0.arctic_n0e1s1w1"},
		{Row: 6, Column: 2, Tag: "t.l0.arctic_n1e0s1w1"},
		{Row: 6, Column: 3, Tag: "t.l0.arctic_n0e0s1w1"},
		{Row: 6, Column: 4, Tag: "t.l0.arctic_n1e1s0w1"},
		{Row: 6, Column: 5, Tag: "t.l0.arctic_n0e1s0w1"},
		{Row: 6, Column: 6, Tag: "t.l0.arctic_n1e0s0w1"},
		{Row: 6, Column: 7, Tag: "t.l0.arctic_n0e0s0w1"},
		{Row: 6, Column: 8, Tag: "t.l0.arctic_n1e1s1w0"},
		{Row: 6, Column: 9, Tag: "t.l0.arctic_n0e1s1w0"},
		{Row: 6, Column: 10, Tag: "t.l0.arctic_n1e0s1w0"},
		{Row: 6, Column: 11, Tag: "t.l0.arctic_n0e0s1w0"},
		{Row: 6, Column: 12, Tag: "t.l0.arctic_n1e1s0w0"},
		{Row: 6, Column: 13, Tag: "t.l0.arctic_n0e1s0w0"},
		{Row: 6, Column: 14, Tag: "t.l0.arctic_n1e0s0w0"},
		{Row: 6, Column: 15, Tag: "t.l0.arctic_n0e0s0w0"},

		{Row: 2, Column: 0, Tag: "t.l1.arctic_n1e1s1w1"},
		{Row: 2, Column: 1, Tag: "t.l1.arctic_n0e1s1w1"},
		{Row: 2, Column: 2, Tag: "t.l1.arctic_n1e0s1w1"},
		{Row: 2, Column: 3, Tag: "t.l1.arctic_n0e0s1w1"},
		{Row: 2, Column: 4, Tag: "t.l1.arctic_n1e1s0w1"},
		{Row: 2, Column: 5, Tag: "t.l1.arctic_n0e1s0w1"},
		{Row: 2, Column: 6, Tag: "t.l1.arctic_n1e0s0w1"},
		{Row: 2, Column: 7, Tag: "t.l1.arctic_n0e0s0w1"},
		{Row: 2, Column: 8, Tag: "t.l1.arctic_n1e1s1w0"},
		{Row: 2, Column: 9, Tag: "t.l1.arctic_n0e1s1w0"},
		{Row: 2, Column: 10, Tag: "t.l1.arctic_n1e0s1w0"},
		{Row: 2, Column: 11, Tag: "t.l1.arctic_n0e0s1w0"},
		{Row: 2, Column: 12, Tag: "t.l1.arctic_n1e1s0w0"},
		{Row: 2, Column: 13, Tag: "t.l1.arctic_n0e1s0w0"},
		{Row: 2, Column: 14, Tag: "t.l1.arctic_n1e0s0w0"},
		{Row: 2, Column: 15, Tag: "t.l1.arctic_n0e0s0w0"},

		// Jungle, and whether terrain to north, south, east, west
		// is more jungle:

		{Row: 3, Column: 0, Tag: "t.l0.jungle_n1e1s1w1"},
		{Row: 3, Column: 1, Tag: "t.l0.jungle_n0e1s1w1"},
		{Row: 3, Column: 2, Tag: "t.l0.jungle_n1e0s1w1"},
		{Row: 3, Column: 3, Tag: "t.l0.jungle_n0e0s1w1"},
		{Row: 3, Column: 4, Tag: "t.l0.jungle_n1e1s0w1"},
		{Row: 3, Column: 5, Tag: "t.l0.jungle_n0e1s0w1"},
		{Row: 3, Column: 6, Tag: "t.l0.jungle_n1e0s0w1"},
		{Row: 3, Column: 7, Tag: "t.l0.jungle_n0e0s0w1"},
		{Row: 3, Column: 8, Tag: "t.l0.jungle_n1e1s1w0"},
		{Row: 3, Column: 9, Tag: "t.l0.jungle_n0e1s1w0"},
		{Row: 3, Column: 10, Tag: "t.l0.jungle_n1e0s1w0"},
		{Row: 3, Column: 11, Tag: "t.l0.jungle_n0e0s1w0"},
		{Row: 3, Column: 12, Tag: "t.l0.jungle_n1e1s0w0"},
		{Row: 3, Column: 13, Tag: "t.l0.jungle_n0e1s0w0"},
		{Row: 3, Column: 14, Tag: "t.l0.jungle_n1e0s0w0"},
		{Row: 3, Column: 15, Tag: "t.l0.jungle_n0e0s0w0"},

		// Plains, and whether terrain to north, south, east, west
		// is more plains:

		{Row: 4, Column: 0, Tag: "t.l0.plains_n1e1s1w1"},
		{Row: 4, Column: 1, Tag: "t.l0.plains_n0e1s1w1"},
		{Row: 4, Column: 2, Tag: "t.l0.plains_n1e0s1w1"},
		{Row: 4, Column: 3, Tag: "t.l0.plains_n0e0s1w1"},
		{Row: 4, Column: 4, Tag: "t.l0.plains_n1e1s0w1"},
		{Row: 4, Column: 5, Tag: "t.l0.plains_n0e1s0w1"},
		{Row: 4, Column: 6, Tag: "t.l0.plains_n1e0s0w1"},
		{Row: 4, Column: 7, Tag: "t.l0.plains_n0e0s0w1"},
		{Row: 4, Column: 8, Tag: "t.l0.plains_n1e1s1w0"},
		{Row: 4, Column: 9, Tag: "t.l0.plains_n0e1s1w0"},
		{Row: 4, Column: 10, Tag: "t.l0.plains_n1e0s1w0"},
		{Row: 4, Column: 11, Tag: "t.l0.plains_n0e0s1w0"},
		{Row: 4, Column: 12, Tag: "t.l0.plains_n1e1s0w0"},
		{Row: 4, Column: 13, Tag: "t.l0.plains_n0e1s0w0"},
		{Row: 4, Column: 14, Tag: "t.l0.plains_n1e0s0w0"},
		{Row: 4, Column: 15, Tag: "t.l0.plains_n0e0s0w0"},

		// Swamp, and whether terrain to north, south, east, west
		// is more swamp:

		{Row: 5, Column: 0, Tag: "t.l0.swamp_n1e1s1w1"},
		{Row: 5, Column: 1, Tag: "t.l0.swamp_n0e1s1w1"},
		{Row: 5, Column: 2, Tag: "t.l0.swamp_n1e0s1w1"},
		{Row: 5, Column: 3, Tag: "t.l0.swamp_n0e0s1w1"},
		{Row: 5, Column: 4, Tag: "t.l0.swamp_n1e1s0w1"},
		{Row: 5, Column: 5, Tag: "t.l0.swamp_n0e1s0w1"},
		{Row: 5, Column: 6, Tag: "t.l0.swamp_n1e0s0w1"},
		{Row: 5, Column: 7, Tag: "t.l0.swamp_n0e0s0w1"},
		{Row: 5, Column: 8, Tag: "t.l0.swamp_n1e1s1w0"},
		{Row: 5, Column: 9, Tag: "t.l0.swamp_n0e1s1w0"},
		{Row: 5, Column: 10, Tag: "t.l0.swamp_n1e0s1w0"},
		{Row: 5, Column: 11, Tag: "t.l0.swamp_n0e0s1w0"},
		{Row: 5, Column: 12, Tag: "t.l0.swamp_n1e1s0w0"},
		{Row: 5, Column: 13, Tag: "t.l0.swamp_n0e1s0w0"},
		{Row: 5, Column: 14, Tag: "t.l0.swamp_n1e0s0w0"},
		{Row: 5, Column: 15, Tag: "t.l0.swamp_n0e0s0w0"},

		// Tundra, and whether terrain to north, south, east, west
		// is more tundra:

		{Row: 6, Column: 0, Tag: "t.l0.tundra_n1e1s1w1"},
		{Row: 6, Column: 1, Tag: "t.l0.tundra_n0e1s1w1"},
		{Row: 6, Column: 2, Tag: "t.l0.tundra_n1e0s1w1"},
		{Row: 6, Column: 3, Tag: "t.l0.tundra_n0e0s1w1"},
		{Row: 6, Column: 4, Tag: "t.l0.tundra_n1e1s0w1"},
		{Row: 6, Column: 5, Tag: "t.l0.tundra_n0e1s0w1"},
		{Row: 6, Column: 6, Tag: "t.l0.tundra_n1e0s0w1"},
		{Row: 6, Column: 7, Tag: "t.l0.tundra_n0e0s0w1"},
		{Row: 6, Column: 8, Tag: "t.l0.tundra_n1e1s1w0"},
		{Row: 6, Column: 9, Tag: "t.l0.tundra_n0e1s1w0"},
		{Row: 6, Column: 10, Tag: "t.l0.tundra_n1e0s1w0"},
		{Row: 6, Column: 11, Tag: "t.l0.tundra_n0e0s1w0"},
		{Row: 6, Column: 12, Tag: "t.l0.tundra_n1e1s0w0"},
		{Row: 6, Column: 13, Tag: "t.l0.tundra_n0e1s0w0"},
		{Row: 6, Column: 14, Tag: "t.l0.tundra_n1e0s0w0"},
		{Row: 6, Column: 15, Tag: "t.l0.tundra_n0e0s0w0"},

		// Ocean, and whether terrain to north, south, east, west
		// is more ocean (else shoreline)

		{Row: 10, Column: 0, Tag: "t.l1.coast_n1e1s1w1"},
		{Row: 10, Column: 1, Tag: "t.l1.coast_n0e1s1w1"},
		{Row: 10, Column: 2, Tag: "t.l1.coast_n1e0s1w1"},
		{Row: 10, Column: 3, Tag: "t.l1.coast_n0e0s1w1"},
		{Row: 10, Column: 4, Tag: "t.l1.coast_n1e1s0w1"},
		{Row: 10, Column: 5, Tag: "t.l1.coast_n0e1s0w1"},
		{Row: 10, Column: 6, Tag: "t.l1.coast_n1e0s0w1"},
		{Row: 10, Column: 7, Tag: "t.l1.coast_n0e0s0w1"},
		{Row: 10, Column: 8, Tag: "t.l1.coast_n1e1s1w0"},
		{Row: 10, Column: 9, Tag: "t.l1.coast_n0e1s1w0"},
		{Row: 10, Column: 10, Tag: "t.l1.coast_n1e0s1w0"},
		{Row: 10, Column: 11, Tag: "t.l1.coast_n0e0s1w0"},
		{Row: 10, Column: 12, Tag: "t.l1.coast_n1e1s0w0"},
		{Row: 10, Column: 13, Tag: "t.l1.coast_n0e1s0w0"},
		{Row: 10, Column: 14, Tag: "t.l1.coast_n1e0s0w0"},
		{Row: 10, Column: 15, Tag: "t.l1.coast_n0e0s0w0"},

		{Row: 10, Column: 0, Tag: "t.l1.floor_n1e1s1w1"},
		{Row: 10, Column: 1, Tag: "t.l1.floor_n0e1s1w1"},
		{Row: 10, Column: 2, Tag: "t.l1.floor_n1e0s1w1"},
		{Row: 10, Column: 3, Tag: "t.l1.floor_n0e0s1w1"},
		{Row: 10, Column: 4, Tag: "t.l1.floor_n1e1s0w1"},
		{Row: 10, Column: 5, Tag: "t.l1.floor_n0e1s0w1"},
		{Row: 10, Column: 6, Tag: "t.l1.floor_n1e0s0w1"},
		{Row: 10, Column: 7, Tag: "t.l1.floor_n0e0s0w1"},
		{Row: 10, Column: 8, Tag: "t.l1.floor_n1e1s1w0"},
		{Row: 10, Column: 9, Tag: "t.l1.floor_n0e1s1w0"},
		{Row: 10, Column: 10, Tag: "t.l1.floor_n1e0s1w0"},
		{Row: 10, Column: 11, Tag: "t.l1.floor_n0e0s1w0"},
		{Row: 10, Column: 12, Tag: "t.l1.floor_n1e1s0w0"},
		{Row: 10, Column: 13, Tag: "t.l1.floor_n0e1s0w0"},
		{Row: 10, Column: 14, Tag: "t.l1.floor_n1e0s0w0"},
		{Row: 10, Column: 15, Tag: "t.l1.floor_n0e0s0w0"},

		{Row: 10, Column: 0, Tag: "t.l1.lake_n1e1s1w1"},
		{Row: 10, Column: 1, Tag: "t.l1.lake_n0e1s1w1"},
		{Row: 10, Column: 2, Tag: "t.l1.lake_n1e0s1w1"},
		{Row: 10, Column: 3, Tag: "t.l1.lake_n0e0s1w1"},
		{Row: 10, Column: 4, Tag: "t.l1.lake_n1e1s0w1"},
		{Row: 10, Column: 5, Tag: "t.l1.lake_n0e1s0w1"},
		{Row: 10, Column: 6, Tag: "t.l1.lake_n1e0s0w1"},
		{Row: 10, Column: 7, Tag: "t.l1.lake_n0e0s0w1"},
		{Row: 10, Column: 8, Tag: "t.l1.lake_n1e1s1w0"},
		{Row: 10, Column: 9, Tag: "t.l1.lake_n0e1s1w0"},
		{Row: 10, Column: 10, Tag: "t.l1.lake_n1e0s1w0"},
		{Row: 10, Column: 11, Tag: "t.l1.lake_n0e0s1w0"},
		{Row: 10, Column: 12, Tag: "t.l1.lake_n1e1s0w0"},
		{Row: 10, Column: 13, Tag: "t.l1.lake_n0e1s0w0"},
		{Row: 10, Column: 14, Tag: "t.l1.lake_n1e0s0w0"},
		{Row: 10, Column: 15, Tag: "t.l1.lake_n0e0s0w0"},

		{Row: 10, Column: 0, Tag: "t.l1.inaccessible_n1e1s1w1"},
		{Row: 10, Column: 1, Tag: "t.l1.inaccessible_n0e1s1w1"},
		{Row: 10, Column: 2, Tag: "t.l1.inaccessible_n1e0s1w1"},
		{Row: 10, Column: 3, Tag: "t.l1.inaccessible_n0e0s1w1"},
		{Row: 10, Column: 4, Tag: "t.l1.inaccessible_n1e1s0w1"},
		{Row: 10, Column: 5, Tag: "t.l1.inaccessible_n0e1s0w1"},
		{Row: 10, Column: 6, Tag: "t.l1.inaccessible_n1e0s0w1"},
		{Row: 10, Column: 7, Tag: "t.l1.inaccessible_n0e0s0w1"},
		{Row: 10, Column: 8, Tag: "t.l1.inaccessible_n1e1s1w0"},
		{Row: 10, Column: 9, Tag: "t.l1.inaccessible_n0e1s1w0"},
		{Row: 10, Column: 10, Tag: "t.l1.inaccessible_n1e0s1w0"},
		{Row: 10, Column: 11, Tag: "t.l1.inaccessible_n0e0s1w0"},
		{Row: 10, Column: 12, Tag: "t.l1.inaccessible_n1e1s0w0"},
		{Row: 10, Column: 13, Tag: "t.l1.inaccessible_n0e1s0w0"},
		{Row: 10, Column: 14, Tag: "t.l1.inaccessible_n1e0s0w0"},
		{Row: 10, Column: 15, Tag: "t.l1.inaccessible_n0e0s0w0"},

		// Ice Shelves
		{Row: 11, Column: 0, Tag: "t.l2.coast_n1e1s1w1"},
		{Row: 11, Column: 1, Tag: "t.l2.coast_n0e1s1w1"},
		{Row: 11, Column: 2, Tag: "t.l2.coast_n1e0s1w1"},
		{Row: 11, Column: 3, Tag: "t.l2.coast_n0e0s1w1"},
		{Row: 11, Column: 4, Tag: "t.l2.coast_n1e1s0w1"},
		{Row: 11, Column: 5, Tag: "t.l2.coast_n0e1s0w1"},
		{Row: 11, Column: 6, Tag: "t.l2.coast_n1e0s0w1"},
		{Row: 11, Column: 7, Tag: "t.l2.coast_n0e0s0w1"},
		{Row: 11, Column: 8, Tag: "t.l2.coast_n1e1s1w0"},
		{Row: 11, Column: 9, Tag: "t.l2.coast_n0e1s1w0"},
		{Row: 11, Column: 10, Tag: "t.l2.coast_n1e0s1w0"},
		{Row: 11, Column: 11, Tag: "t.l2.coast_n0e0s1w0"},
		{Row: 11, Column: 12, Tag: "t.l2.coast_n1e1s0w0"},
		{Row: 11, Column: 13, Tag: "t.l2.coast_n0e1s0w0"},
		{Row: 11, Column: 14, Tag: "t.l2.coast_n1e0s0w0"},
		{Row: 11, Column: 15, Tag: "t.l2.coast_n0e0s0w0"},

		{Row: 11, Column: 0, Tag: "t.l2.floor_n1e1s1w1"},
		{Row: 11, Column: 1, Tag: "t.l2.floor_n0e1s1w1"},
		{Row: 11, Column: 2, Tag: "t.l2.floor_n1e0s1w1"},
		{Row: 11, Column: 3, Tag: "t.l2.floor_n0e0s1w1"},
		{Row: 11, Column: 4, Tag: "t.l2.floor_n1e1s0w1"},
		{Row: 11, Column: 5, Tag: "t.l2.floor_n0e1s0w1"},
		{Row: 11, Column: 6, Tag: "t.l2.floor_n1e0s0w1"},
		{Row: 11, Column: 7, Tag: "t.l2.floor_n0e0s0w1"},
		{Row: 11, Column: 8, Tag: "t.l2.floor_n1e1s1w0"},
		{Row: 11, Column: 9, Tag: "t.l2.floor_n0e1s1w0"},
		{Row: 11, Column: 10, Tag: "t.l2.floor_n1e0s1w0"},
		{Row: 11, Column: 11, Tag: "t.l2.floor_n0e0s1w0"},
		{Row: 11, Column: 12, Tag: "t.l2.floor_n1e1s0w0"},
		{Row: 11, Column: 13, Tag: "t.l2.floor_n0e1s0w0"},
		{Row: 11, Column: 14, Tag: "t.l2.floor_n1e0s0w0"},
		{Row: 11, Column: 15, Tag: "t.l2.floor_n0e0s0w0"},

		{Row: 11, Column: 0, Tag: "t.l2.lake_n1e1s1w1"},
		{Row: 11, Column: 1, Tag: "t.l2.lake_n0e1s1w1"},
		{Row: 11, Column: 2, Tag: "t.l2.lake_n1e0s1w1"},
		{Row: 11, Column: 3, Tag: "t.l2.lake_n0e0s1w1"},
		{Row: 11, Column: 4, Tag: "t.l2.lake_n1e1s0w1"},
		{Row: 11, Column: 5, Tag: "t.l2.lake_n0e1s0w1"},
		{Row: 11, Column: 6, Tag: "t.l2.lake_n1e0s0w1"},
		{Row: 11, Column: 7, Tag: "t.l2.lake_n0e0s0w1"},
		{Row: 11, Column: 8, Tag: "t.l2.lake_n1e1s1w0"},
		{Row: 11, Column: 9, Tag: "t.l2.lake_n0e1s1w0"},
		{Row: 11, Column: 10, Tag: "t.l2.lake_n1e0s1w0"},
		{Row: 11, Column: 11, Tag: "t.l2.lake_n0e0s1w0"},
		{Row: 11, Column: 12, Tag: "t.l2.lake_n1e1s0w0"},
		{Row: 11, Column: 13, Tag: "t.l2.lake_n0e1s0w0"},
		{Row: 11, Column: 14, Tag: "t.l2.lake_n1e0s0w0"},
		{Row: 11, Column: 15, Tag: "t.l2.lake_n0e0s0w0"},

		{Row: 11, Column: 0, Tag: "t.l2.inaccessible_n1e1s1w1"},
		{Row: 11, Column: 1, Tag: "t.l2.inaccessible_n0e1s1w1"},
		{Row: 11, Column: 2, Tag: "t.l2.inaccessible_n1e0s1w1"},
		{Row: 11, Column: 3, Tag: "t.l2.inaccessible_n0e0s1w1"},
		{Row: 11, Column: 4, Tag: "t.l2.inaccessible_n1e1s0w1"},
		{Row: 11, Column: 5, Tag: "t.l2.inaccessible_n0e1s0w1"},
		{Row: 11, Column: 6, Tag: "t.l2.inaccessible_n1e0s0w1"},
		{Row: 11, Column: 7, Tag: "t.l2.inaccessible_n0e0s0w1"},
		{Row: 11, Column: 8, Tag: "t.l2.inaccessible_n1e1s1w0"},
		{Row: 11, Column: 9, Tag: "t.l2.inaccessible_n0e1s1w0"},
		{Row: 11, Column: 10, Tag: "t.l2.inaccessible_n1e0s1w0"},
		{Row: 11, Column: 11, Tag: "t.l2.inaccessible_n0e0s1w0"},
		{Row: 11, Column: 12, Tag: "t.l2.inaccessible_n1e1s0w0"},
		{Row: 11, Column: 13, Tag: "t.l2.inaccessible_n0e1s0w0"},
		{Row: 11, Column: 14, Tag: "t.l2.inaccessible_n1e0s0w0"},
		{Row: 11, Column: 15, Tag: "t.l2.inaccessible_n0e0s0w0"},

		// Darkness (unexplored) to north, south, east, west
		{Row: 12, Column: 0, Tag: "mask.tile"},
		{Row: 12, Column: 1, Tag: "tx.darkness_n1e0s0w0"},
		{Row: 12, Column: 2, Tag: "tx.darkness_n0e1s0w0"},
		{Row: 12, Column: 3, Tag: "tx.darkness_n1e1s0w0"},
		{Row: 12, Column: 4, Tag: "tx.darkness_n0e0s1w0"},
		{Row: 12, Column: 5, Tag: "tx.darkness_n1e0s1w0"},
		{Row: 12, Column: 6, Tag: "tx.darkness_n0e1s1w0"},
		{Row: 12, Column: 7, Tag: "tx.darkness_n1e1s1w0"},
		{Row: 12, Column: 8, Tag: "tx.darkness_n0e0s0w1"},
		{Row: 12, Column: 9, Tag: "tx.darkness_n1e0s0w1"},
		{Row: 12, Column: 10, Tag: "tx.darkness_n0e1s0w1"},
		{Row: 12, Column: 11, Tag: "tx.darkness_n1e1s0w1"},
		{Row: 12, Column: 12, Tag: "tx.darkness_n0e0s1w1"},
		{Row: 12, Column: 13, Tag: "tx.darkness_n1e0s1w1"},
		{Row: 12, Column: 14, Tag: "tx.darkness_n0e1s1w1"},
		{Row: 12, Column: 15, Tag: "tx.darkness_n1e1s1w1"},
		{Row: 12, Column: 16, Tag: "tx.fog"},

		// Rivers (as special type), and whether north, south, east, west
		// also has river or is ocean:
		{Row: 13, Column: 0, Tag: "road.river_s_n0e0s0w0"},
		{Row: 13, Column: 1, Tag: "road.river_s_n1e0s0w0"},
		{Row: 13, Column: 2, Tag: "road.river_s_n0e1s0w0"},
		{Row: 13, Column: 3, Tag: "road.river_s_n1e1s0w0"},
		{Row: 13, Column: 4, Tag: "road.river_s_n0e0s1w0"},
		{Row: 13, Column: 5, Tag: "road.river_s_n1e0s1w0"},
		{Row: 13, Column: 6, Tag: "road.river_s_n0e1s1w0"},
		{Row: 13, Column: 7, Tag: "road.river_s_n1e1s1w0"},
		{Row: 13, Column: 8, Tag: "road.river_s_n0e0s0w1"},
		{Row: 13, Column: 9, Tag: "road.river_s_n1e0s0w1"},
		{Row: 13, Column: 10, Tag: "road.river_s_n0e1s0w1"},
		{Row: 13, Column: 11, Tag: "road.river_s_n1e1s0w1"},
		{Row: 13, Column: 12, Tag: "road.river_s_n0e0s1w1"},
		{Row: 13, Column: 13, Tag: "road.river_s_n1e0s1w1"},
		{Row: 13, Column: 14, Tag: "road.river_s_n0e1s1w1"},
		{Row: 13, Column: 15, Tag: "road.river_s_n1e1s1w1"},

		// River outlets, river to north, south, east, west
		{Row: 14, Column: 12, Tag: "road.river_outlet_n"},
		{Row: 14, Column: 13, Tag: "road.river_outlet_w"},
		{Row: 14, Column: 14, Tag: "road.river_outlet_s"},
		{Row: 14, Column: 15, Tag: "road.river_outlet_e"},

		// Terrain special resources:
		{Row: 14, Column: 0, Tag: "ts.spice"},
		{Row: 14, Column: 1, Tag: "ts.furs"},
		{Row: 14, Column: 2, Tag: "ts.peat"},
		{Row: 14, Column: 3, Tag: "ts.arctic_ivory"},
		{Row: 14, Column: 4, Tag: "ts.fruit"},
		{Row: 14, Column: 5, Tag: "ts.iron"},
		{Row: 14, Column: 6, Tag: "ts.whales"},
		{Row: 14, Column: 7, Tag: "ts.wheat"},
		{Row: 14, Column: 8, Tag: "ts.pheasant"},
		{Row: 14, Column: 9, Tag: "ts.buffalo"},
		{Row: 14, Column: 10, Tag: "ts.silk"},
		{Row: 14, Column: 11, Tag: "ts.wine"},

		{Row: 15, Column: 0, Tag: "ts.seals"},
		{Row: 15, Column: 1, Tag: "ts.oasis"},
		{Row: 15, Column: 2, Tag: "ts.forest_game"},
		{Row: 15, Column: 3, Tag: "ts.grassland_resources"},
		{Row: 15, Column: 4, Tag: "ts.coal"},
		{Row: 15, Column: 5, Tag: "ts.gems"},
		{Row: 15, Column: 6, Tag: "ts.gold"},
		{Row: 15, Column: 7, Tag: "ts.fish"},
		{Row: 15, Column: 8, Tag: "ts.horses"},
		{Row: 15, Column: 9, Tag: "ts.river_resources"},
		{Row: 15, Column: 10, Tag: "ts.oilColumn: "},
		{Row: 15, Column: 10, Tag: "ts.arctic_oil"},
		{Row: 15, Column: 11, Tag: "ts.tundra_game"},

		// Terrain Strategic Resources
		{Row: 15, Column: 12, Tag: "ts.aluminum"},
		{Row: 15, Column: 13, Tag: "ts.uranium"},
		{Row: 15, Column: 14, Tag: "ts.saltpeter"},
		{Row: 15, Column: 15, Tag: "ts.elephant"},

		// Terrain improvements and similar:
		{Row: 16, Column: 0, Tag: "tx.farmland"},
		{Row: 16, Column: 1, Tag: "tx.irrigation"},
		{Row: 16, Column: 2, Tag: "tx.mine"},
		{Row: 16, Column: 3, Tag: "tx.oil_mine"},
		{Row: 16, Column: 4, Tag: "tx.pollution"},
		{Row: 16, Column: 5, Tag: "tx.fallout"},
		{Row: 16, Column: 13, Tag: "tx.oil_rig"},

		// Bases
		{Row: 16, Column: 6, Tag: "base.buoy_mg"},
		{Row: 16, Column: 7, Tag: "extra.ruins_mg"},
		{Row: 16, Column: 8, Tag: "tx.village"},
		{Row: 16, Column: 9, Tag: "base.airstrip_mg"},
		{Row: 16, Column: 10, Tag: "base.airbase_mg"},
		{Row: 16, Column: 11, Tag: "base.outpost_mg"},
		{Row: 16, Column: 12, Tag: "base.fortress_bg"},

		// Numbers: city size: (also used for goto)
		{Row: 17, Column: 0, Tag: "city.size_0"},
		{Row: 17, Column: 1, Tag: "city.size_1"},
		{Row: 17, Column: 2, Tag: "city.size_2"},
		{Row: 17, Column: 3, Tag: "city.size_3"},
		{Row: 17, Column: 4, Tag: "city.size_4"},
		{Row: 17, Column: 5, Tag: "city.size_5"},
		{Row: 17, Column: 6, Tag: "city.size_6"},
		{Row: 17, Column: 7, Tag: "city.size_7"},
		{Row: 17, Column: 8, Tag: "city.size_8"},
		{Row: 17, Column: 9, Tag: "city.size_9"},
		{Row: 18, Column: 0, Tag: "city.size_00"},
		{Row: 18, Column: 1, Tag: "city.size_10"},
		{Row: 18, Column: 2, Tag: "city.size_20"},
		{Row: 18, Column: 3, Tag: "city.size_30"},
		{Row: 18, Column: 4, Tag: "city.size_40"},
		{Row: 18, Column: 5, Tag: "city.size_50"},
		{Row: 18, Column: 6, Tag: "city.size_60"},
		{Row: 18, Column: 7, Tag: "city.size_70"},
		{Row: 18, Column: 8, Tag: "city.size_80"},
		{Row: 18, Column: 9, Tag: "city.size_90"},
		{Row: 19, Column: 1, Tag: "city.size_100"},
		{Row: 19, Column: 2, Tag: "city.size_200"},
		{Row: 19, Column: 3, Tag: "city.size_300"},
		{Row: 19, Column: 4, Tag: "city.size_400"},
		{Row: 19, Column: 5, Tag: "city.size_500"},
		{Row: 19, Column: 6, Tag: "city.size_600"},
		{Row: 19, Column: 7, Tag: "city.size_700"},
		{Row: 19, Column: 8, Tag: "city.size_800"},
		{Row: 19, Column: 9, Tag: "city.size_900"},

		// Numbers: city tile food/shields/trade y/g/b

		{Row: 20, Column: 0, Tag: "city.t_food_0"},
		{Row: 20, Column: 1, Tag: "city.t_food_1"},
		{Row: 20, Column: 2, Tag: "city.t_food_2"},
		{Row: 20, Column: 3, Tag: "city.t_food_3"},
		{Row: 20, Column: 4, Tag: "city.t_food_4"},
		{Row: 20, Column: 5, Tag: "city.t_food_5"},
		{Row: 20, Column: 6, Tag: "city.t_food_6"},
		{Row: 20, Column: 7, Tag: "city.t_food_7"},
		{Row: 20, Column: 8, Tag: "city.t_food_8"},
		{Row: 20, Column: 9, Tag: "city.t_food_9"},

		{Row: 21, Column: 0, Tag: "city.t_shields_0"},
		{Row: 21, Column: 1, Tag: "city.t_shields_1"},
		{Row: 21, Column: 2, Tag: "city.t_shields_2"},
		{Row: 21, Column: 3, Tag: "city.t_shields_3"},
		{Row: 21, Column: 4, Tag: "city.t_shields_4"},
		{Row: 21, Column: 5, Tag: "city.t_shields_5"},
		{Row: 21, Column: 6, Tag: "city.t_shields_6"},
		{Row: 21, Column: 7, Tag: "city.t_shields_7"},
		{Row: 21, Column: 8, Tag: "city.t_shields_8"},
		{Row: 21, Column: 9, Tag: "city.t_shields_9"},

		{Row: 22, Column: 0, Tag: "city.t_trade_0"},
		{Row: 22, Column: 1, Tag: "city.t_trade_1"},
		{Row: 22, Column: 2, Tag: "city.t_trade_2"},
		{Row: 22, Column: 3, Tag: "city.t_trade_3"},
		{Row: 22, Column: 4, Tag: "city.t_trade_4"},
		{Row: 22, Column: 5, Tag: "city.t_trade_5"},
		{Row: 22, Column: 6, Tag: "city.t_trade_6"},
		{Row: 22, Column: 7, Tag: "city.t_trade_7"},
		{Row: 22, Column: 8, Tag: "city.t_trade_8"},
		{Row: 22, Column: 9, Tag: "city.t_trade_9"},

		// Unit Misc:
		{Row: 5, Column: 16, Tag: "unit.tired"},
		{Row: 5, Column: 16, Tag: "unit.lowfuel"},
		{Row: 5, Column: 17, Tag: "unit.loaded"},
		{Row: 5, Column: 18, Tag: "user.attention"},
		{Row: 5, Column: 18, Tag: "user.infratile"}, // Variously crosshair/red-square/arrows
		{Row: 5, Column: 19, Tag: "unit.stack"},

		// Goto path:
		{Row: 1, Column: 16, Tag: "path.step"},         // turn boundary within path
		{Row: 2, Column: 16, Tag: "path.exhausted_mp"}, // tip of path, no MP left
		{Row: 3, Column: 16, Tag: "path.normal"},       // tip of path with MP remaining
		{Row: 4, Column: 16, Tag: "path.waypoint"},

		// Unit activity letters:  (note unit icons have just "u.")
		{Row: 6, Column: 17, Tag: "unit.auto_attack"},
		{Row: 6, Column: 17, Tag: "unit.auto_worker"},
		{Row: 6, Column: 18, Tag: "unit.connect"},
		{Row: 6, Column: 19, Tag: "unit.auto_explore"},

		{Row: 7, Column: 16, Tag: "unit.fortifying"},
		{Row: 7, Column: 17, Tag: "unit.fortified"},
		{Row: 7, Column: 18, Tag: "unit.sentry"},
		{Row: 7, Column: 19, Tag: "unit.patrol"},

		{Row: 8, Column: 16, Tag: "unit.plant"},
		{Row: 8, Column: 17, Tag: "unit.cultivate"},
		{Row: 8, Column: 17, Tag: "unit.irrigate"}, // For rulesets still using this tag
		{Row: 8, Column: 18, Tag: "unit.transform"},
		{Row: 8, Column: 19, Tag: "unit.pillage"},

		{Row: 9, Column: 16, Tag: "unit.pollution"},
		{Row: 9, Column: 17, Tag: "unit.fallout"},
		{Row: 9, Column: 18, Tag: "unit.convert"},
		{Row: 9, Column: 19, Tag: "unit.goto"},


		// Unit Activities

		{Row: 10, Column: 16, Tag: "unit.airstrip"},
		{Row: 10, Column: 17, Tag: "unit.outpost"},
		{Row: 10, Column: 18, Tag: "unit.airbase"},
		{Row: 10, Column: 19, Tag: "unit.fortress"},
		{Row: 11, Column: 19, Tag: "unit.buoy"},

		// Road Activities
		{Row: 11, Column: 16, Tag: "unit.road"},
		{Row: 11, Column: 17, Tag: "unit.rail"},
		{Row: 11, Column: 18, Tag: "unit.maglev"},

		// Unit hit-point bars: approx percent of hp remaining
		{Row: 17, Column: 10, Tag: "unit.hp_100"},
		{Row: 17, Column: 11, Tag: "unit.hp_90"},
		{Row: 17, Column: 12, Tag: "unit.hp_80"},
		{Row: 17, Column: 13, Tag: "unit.hp_70"},
		{Row: 17, Column: 14, Tag: "unit.hp_60"},
		{Row: 17, Column: 15, Tag: "unit.hp_50"},
		{Row: 17, Column: 16, Tag: "unit.hp_40"},
		{Row: 17, Column: 17, Tag: "unit.hp_30"},
		{Row: 17, Column: 18, Tag: "unit.hp_20"},
		{Row: 17, Column: 19, Tag: "unit.hp_10"},
		{Row: 18, Column: 19, Tag: "unit.hp_0"},

		// Veteran Levels: up to 9 military honors for experienced units
		{Row: 18, Column: 10, Tag: "unit.vet_1"},
		{Row: 18, Column: 11, Tag: "unit.vet_2"},
		{Row: 18, Column: 12, Tag: "unit.vet_3"},
		{Row: 18, Column: 13, Tag: "unit.vet_4"},
		{Row: 18, Column: 14, Tag: "unit.vet_5"},
		{Row: 18, Column: 15, Tag: "unit.vet_6"},
		{Row: 18, Column: 16, Tag: "unit.vet_7"},
		{Row: 18, Column: 17, Tag: "unit.vet_8"},
		{Row: 18, Column: 18, Tag: "unit.vet_9"},

		// Unit upkeep in city dialog:
		// These should probably be handled differently and have
		// a different size...
		{Row: 19, Column: 10, Tag: "upkeep.shield"},
		{Row: 19, Column: 11, Tag: "upkeep.shield2"},
		{Row: 19, Column: 12, Tag: "upkeep.shield3"},
		{Row: 19, Column: 13, Tag: "upkeep.shield4"},
		{Row: 19, Column: 14, Tag: "upkeep.shield5"},
		{Row: 19, Column: 15, Tag: "upkeep.shield6"},
		{Row: 19, Column: 16, Tag: "upkeep.shield7"},
		{Row: 19, Column: 17, Tag: "upkeep.shield8"},
		{Row: 19, Column: 18, Tag: "upkeep.shield9"},
		{Row: 19, Column: 19, Tag: "upkeep.shield10"},

		{Row: 20, Column: 10, Tag: "upkeep.unhappy"},
		{Row: 20, Column: 11, Tag: "upkeep.unhappy2"},
		{Row: 20, Column: 12, Tag: "upkeep.unhappy3"},
		{Row: 20, Column: 13, Tag: "upkeep.unhappy4"},
		{Row: 20, Column: 14, Tag: "upkeep.unhappy5"},
		{Row: 20, Column: 15, Tag: "upkeep.unhappy6"},
		{Row: 20, Column: 16, Tag: "upkeep.unhappy7"},
		{Row: 20, Column: 17, Tag: "upkeep.unhappy8"},
		{Row: 20, Column: 18, Tag: "upkeep.unhappy9"},
		{Row: 20, Column: 19, Tag: "upkeep.unhappy10"},

		{Row: 21, Column: 10, Tag: "upkeep.food"},
		{Row: 21, Column: 11, Tag: "upkeep.food2"},
		{Row: 21, Column: 12, Tag: "upkeep.food3"},
		{Row: 21, Column: 13, Tag: "upkeep.food4"},
		{Row: 21, Column: 14, Tag: "upkeep.food5"},
		{Row: 21, Column: 15, Tag: "upkeep.food6"},
		{Row: 21, Column: 16, Tag: "upkeep.food7"},
		{Row: 21, Column: 17, Tag: "upkeep.food8"},
		{Row: 21, Column: 18, Tag: "upkeep.food9"},
		{Row: 21, Column: 19, Tag: "upkeep.food10"},

		{Row: 22, Column: 10, Tag: "upkeep.gold"},
		{Row: 22, Column: 11, Tag: "upkeep.gold2"},
		{Row: 22, Column: 12, Tag: "upkeep.gold3"},
		{Row: 22, Column: 13, Tag: "upkeep.gold4"},
		{Row: 22, Column: 14, Tag: "upkeep.gold5"},
		{Row: 22, Column: 15, Tag: "upkeep.gold6"},
		{Row: 22, Column: 16, Tag: "upkeep.gold7"},
		{Row: 22, Column: 17, Tag: "upkeep.gold8"},
		{Row: 22, Column: 18, Tag: "upkeep.gold9"},
		{Row: 22, Column: 19, Tag: "upkeep.gold10"},
	},
}

// GridNuke Data from the [grid_nuke] section
var GridNuke = Tile{
	XTopLeft: 510,
	YTopLeft: 30,
	DX:       90,
	DY:       90,
	Tiles: []TileSpec{
		{Row: 0, Column: 0, Tag: "explode.nuke"},
	},
}

// GridOcean Data from the [grid_ocean] section
var GridOcean = Tile{
	XTopLeft: 0,
	YTopLeft: 210,
	DX:       15,
	DY:       15,
	Tiles: []TileSpec{

		// coast cell sprites.  See doc/README.graphics

		{Row: 0, Column: 0, Tag: "t.l0.coast_cell_u000"},
		{Row: 0, Column: 0, Tag: "t.l0.coast_cell_u001"},
		{Row: 0, Column: 0, Tag: "t.l0.coast_cell_u100"},
		{Row: 0, Column: 0, Tag: "t.l0.coast_cell_u101"},

		{Row: 0, Column: 1, Tag: "t.l0.coast_cell_u010"},
		{Row: 0, Column: 1, Tag: "t.l0.coast_cell_u011"},
		{Row: 0, Column: 1, Tag: "t.l0.coast_cell_u110"},
		{Row: 0, Column: 1, Tag: "t.l0.coast_cell_u111"},

		{Row: 0, Column: 2, Tag: "t.l0.coast_cell_r000"},
		{Row: 0, Column: 2, Tag: "t.l0.coast_cell_r001"},
		{Row: 0, Column: 2, Tag: "t.l0.coast_cell_r100"},
		{Row: 0, Column: 2, Tag: "t.l0.coast_cell_r101"},

		{Row: 0, Column: 3, Tag: "t.l0.coast_cell_r010"},
		{Row: 0, Column: 3, Tag: "t.l0.coast_cell_r011"},
		{Row: 0, Column: 3, Tag: "t.l0.coast_cell_r110"},
		{Row: 0, Column: 3, Tag: "t.l0.coast_cell_r111"},

		{Row: 0, Column: 4, Tag: "t.l0.coast_cell_l000"},
		{Row: 0, Column: 4, Tag: "t.l0.coast_cell_l001"},
		{Row: 0, Column: 4, Tag: "t.l0.coast_cell_l100"},
		{Row: 0, Column: 4, Tag: "t.l0.coast_cell_l101"},

		{Row: 0, Column: 5, Tag: "t.l0.coast_cell_l010"},
		{Row: 0, Column: 5, Tag: "t.l0.coast_cell_l011"},
		{Row: 0, Column: 5, Tag: "t.l0.coast_cell_l110"},
		{Row: 0, Column: 5, Tag: "t.l0.coast_cell_l111"},

		{Row: 0, Column: 6, Tag: "t.l0.coast_cell_d000"},
		{Row: 0, Column: 6, Tag: "t.l0.coast_cell_d001"},
		{Row: 0, Column: 6, Tag: "t.l0.coast_cell_d100"},
		{Row: 0, Column: 6, Tag: "t.l0.coast_cell_d101"},

		{Row: 0, Column: 7, Tag: "t.l0.coast_cell_d010"},
		{Row: 0, Column: 7, Tag: "t.l0.coast_cell_d011"},
		{Row: 0, Column: 7, Tag: "t.l0.coast_cell_d110"},
		{Row: 0, Column: 7, Tag: "t.l0.coast_cell_d111"},

		// Deep Ocean fallback to Ocean tiles
		{Row: 0, Column: 8, Tag: "t.l0.floor_cell_u000"},
		{Row: 0, Column: 8, Tag: "t.l0.floor_cell_u001"},
		{Row: 0, Column: 8, Tag: "t.l0.floor_cell_u100"},
		{Row: 0, Column: 8, Tag: "t.l0.floor_cell_u101"},

		{Row: 0, Column: 9, Tag: "t.l0.floor_cell_u010"},
		{Row: 0, Column: 9, Tag: "t.l0.floor_cell_u011"},
		{Row: 0, Column: 9, Tag: "t.l0.floor_cell_u110"},
		{Row: 0, Column: 9, Tag: "t.l0.floor_cell_u111"},

		{Row: 0, Column: 10, Tag: "t.l0.floor_cell_r000"},
		{Row: 0, Column: 10, Tag: "t.l0.floor_cell_r001"},
		{Row: 0, Column: 10, Tag: "t.l0.floor_cell_r100"},
		{Row: 0, Column: 10, Tag: "t.l0.floor_cell_r101"},

		{Row: 0, Column: 11, Tag: "t.l0.floor_cell_r010"},
		{Row: 0, Column: 11, Tag: "t.l0.floor_cell_r011"},
		{Row: 0, Column: 11, Tag: "t.l0.floor_cell_r110"},
		{Row: 0, Column: 11, Tag: "t.l0.floor_cell_r111"},

		{Row: 0, Column: 12, Tag: "t.l0.floor_cell_l000"},
		{Row: 0, Column: 12, Tag: "t.l0.floor_cell_l001"},
		{Row: 0, Column: 12, Tag: "t.l0.floor_cell_l100"},
		{Row: 0, Column: 12, Tag: "t.l0.floor_cell_l101"},

		{Row: 0, Column: 13, Tag: "t.l0.floor_cell_l010"},
		{Row: 0, Column: 13, Tag: "t.l0.floor_cell_l011"},
		{Row: 0, Column: 13, Tag: "t.l0.floor_cell_l110"},
		{Row: 0, Column: 13, Tag: "t.l0.floor_cell_l111"},

		{Row: 0, Column: 14, Tag: "t.l0.floor_cell_d000"},
		{Row: 0, Column: 14, Tag: "t.l0.floor_cell_d001"},
		{Row: 0, Column: 14, Tag: "t.l0.floor_cell_d100"},
		{Row: 0, Column: 14, Tag: "t.l0.floor_cell_d101"},

		{Row: 0, Column: 15, Tag: "t.l0.floor_cell_d010"},
		{Row: 0, Column: 15, Tag: "t.l0.floor_cell_d011"},
		{Row: 0, Column: 15, Tag: "t.l0.floor_cell_d110"},
		{Row: 0, Column: 15, Tag: "t.l0.floor_cell_d111"},

		// Lake fallback to Ocean tiles
		{Row: 0, Column: 16, Tag: "t.l0.lake_cell_u000"},
		{Row: 0, Column: 16, Tag: "t.l0.lake_cell_u001"},
		{Row: 0, Column: 16, Tag: "t.l0.lake_cell_u100"},
		{Row: 0, Column: 16, Tag: "t.l0.lake_cell_u101"},

		{Row: 0, Column: 17, Tag: "t.l0.lake_cell_u010"},
		{Row: 0, Column: 17, Tag: "t.l0.lake_cell_u011"},
		{Row: 0, Column: 17, Tag: "t.l0.lake_cell_u110"},
		{Row: 0, Column: 17, Tag: "t.l0.lake_cell_u111"},

		{Row: 0, Column: 18, Tag: "t.l0.lake_cell_r000"},
		{Row: 0, Column: 18, Tag: "t.l0.lake_cell_r001"},
		{Row: 0, Column: 18, Tag: "t.l0.lake_cell_r100"},
		{Row: 0, Column: 18, Tag: "t.l0.lake_cell_r101"},

		{Row: 0, Column: 19, Tag: "t.l0.lake_cell_r010"},
		{Row: 0, Column: 19, Tag: "t.l0.lake_cell_r011"},
		{Row: 0, Column: 19, Tag: "t.l0.lake_cell_r110"},
		{Row: 0, Column: 19, Tag: "t.l0.lake_cell_r111"},

		{Row: 0, Column: 20, Tag: "t.l0.lake_cell_l000"},
		{Row: 0, Column: 20, Tag: "t.l0.lake_cell_l001"},
		{Row: 0, Column: 20, Tag: "t.l0.lake_cell_l100"},
		{Row: 0, Column: 20, Tag: "t.l0.lake_cell_l101"},

		{Row: 0, Column: 21, Tag: "t.l0.lake_cell_l010"},
		{Row: 0, Column: 21, Tag: "t.l0.lake_cell_l011"},
		{Row: 0, Column: 21, Tag: "t.l0.lake_cell_l110"},
		{Row: 0, Column: 21, Tag: "t.l0.lake_cell_l111"},

		{Row: 0, Column: 22, Tag: "t.l0.lake_cell_d000"},
		{Row: 0, Column: 22, Tag: "t.l0.lake_cell_d001"},
		{Row: 0, Column: 22, Tag: "t.l0.lake_cell_d100"},
		{Row: 0, Column: 22, Tag: "t.l0.lake_cell_d101"},

		{Row: 0, Column: 23, Tag: "t.l0.lake_cell_d010"},
		{Row: 0, Column: 23, Tag: "t.l0.lake_cell_d011"},
		{Row: 0, Column: 23, Tag: "t.l0.lake_cell_d110"},
		{Row: 0, Column: 23, Tag: "t.l0.lake_cell_d111"},

		// Inaccessible fallback to Ocean tiles
		{Row: 0, Column: 24, Tag: "t.l0.inaccessible_cell_u000"},
		{Row: 0, Column: 24, Tag: "t.l0.inaccessible_cell_u001"},
		{Row: 0, Column: 24, Tag: "t.l0.inaccessible_cell_u100"},
		{Row: 0, Column: 24, Tag: "t.l0.inaccessible_cell_u101"},

		{Row: 0, Column: 25, Tag: "t.l0.inaccessible_cell_u010"},
		{Row: 0, Column: 25, Tag: "t.l0.inaccessible_cell_u011"},
		{Row: 0, Column: 25, Tag: "t.l0.inaccessible_cell_u110"},
		{Row: 0, Column: 25, Tag: "t.l0.inaccessible_cell_u111"},

		{Row: 0, Column: 26, Tag: "t.l0.inaccessible_cell_r000"},
		{Row: 0, Column: 26, Tag: "t.l0.inaccessible_cell_r001"},
		{Row: 0, Column: 26, Tag: "t.l0.inaccessible_cell_r100"},
		{Row: 0, Column: 26, Tag: "t.l0.inaccessible_cell_r101"},

		{Row: 0, Column: 27, Tag: "t.l0.inaccessible_cell_r010"},
		{Row: 0, Column: 27, Tag: "t.l0.inaccessible_cell_r011"},
		{Row: 0, Column: 27, Tag: "t.l0.inaccessible_cell_r110"},
		{Row: 0, Column: 27, Tag: "t.l0.inaccessible_cell_r111"},

		{Row: 0, Column: 28, Tag: "t.l0.inaccessible_cell_l000"},
		{Row: 0, Column: 28, Tag: "t.l0.inaccessible_cell_l001"},
		{Row: 0, Column: 28, Tag: "t.l0.inaccessible_cell_l100"},
		{Row: 0, Column: 28, Tag: "t.l0.inaccessible_cell_l101"},

		{Row: 0, Column: 29, Tag: "t.l0.inaccessible_cell_l010"},
		{Row: 0, Column: 29, Tag: "t.l0.inaccessible_cell_l011"},
		{Row: 0, Column: 29, Tag: "t.l0.inaccessible_cell_l110"},
		{Row: 0, Column: 29, Tag: "t.l0.inaccessible_cell_l111"},

		{Row: 0, Column: 30, Tag: "t.l0.inaccessible_cell_d000"},
		{Row: 0, Column: 30, Tag: "t.l0.inaccessible_cell_d001"},
		{Row: 0, Column: 30, Tag: "t.l0.inaccessible_cell_d100"},
		{Row: 0, Column: 30, Tag: "t.l0.inaccessible_cell_d101"},

		{Row: 0, Column: 31, Tag: "t.l0.inaccessible_cell_d010"},
		{Row: 0, Column: 31, Tag: "t.l0.inaccessible_cell_d011"},
		{Row: 0, Column: 31, Tag: "t.l0.inaccessible_cell_d110"},
		{Row: 0, Column: 31, Tag: "t.l0.inaccessible_cell_d111"},
	},
}
