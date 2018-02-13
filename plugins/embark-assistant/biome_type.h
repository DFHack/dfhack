//  world_coord_x/y is the location of the tile "owning" the biome, while world_ref_coord_y is the
//  location of the tile the biome appears on. They differ when a mid level tile ("region tile")
//  refers to a neighboring tile for the biome parameters. The difference can affect the tropicality
//  determination. Since Tropicality is determined by latitude, the x coordinate of the reference is
//  omitted.
//
int get_biome_type(int world_coord_x, int world_coord_y, int world_ref_coord_y);
