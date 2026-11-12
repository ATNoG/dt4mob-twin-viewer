#pragma once

/**
 * @file
 * @brief Header-only geotile (Web Mercator tile and quadkey) math shared by the backend
 *        service and entity structs.
 * @ingroup core
 */

#include "CoreMinimal.h"

/**
 * @brief Static geotile and quadkey helpers with no UObject dependency.
 *
 * A geotile is a 64-bit quadkey integer for a Web Mercator (EPSG:3857, OSM slippy map) tile at
 * a given zoom level, two bits per level. Things in Ditto carry one in `attributes/geotile`;
 * TRACI-simulated vehicles encode it at zoom 18 (checked against a live "traci" vehicle's
 * value), while static infrastructure imports use zoom 31 (see
 * ADT4MOBGamemode::GeotileStorageZooms).
 *
 * UBackendConnectionService::GetTileXY, GetQuadkeyFromXY, GetQuadkey and GetTileBounds forward
 * to these functions, so both give identical results.
 *
 * @ingroup core
 */
struct FGeotileUtils
{
    /**
     * @brief Converts a geographic point to slippy map tile X/Y indices at a zoom level.
     *
     * @param Lat   Latitude in decimal degrees. Values past the Web Mercator limit (about
     *              85.05 degrees) end up clamped to the first or last tile row.
     * @param Lon   Longitude in decimal degrees, -180 to 180.
     * @param Zoom  Zoom level; the grid is 2^Zoom tiles wide and tall.
     * @param OutX  Receives the tile column, clamped to 0 to 2^Zoom - 1 (0 = 180 degrees west).
     * @param OutY  Receives the tile row, clamped to 0 to 2^Zoom - 1 (0 = northernmost row).
     */
    static void LatLonToTileXY(double Lat, double Lon, int32 Zoom, int64& OutX, int64& OutY)
    {
        const int64  TileCount = int64(1) << Zoom;
        const double LatRad    = FMath::DegreesToRadians(Lat);
        OutX = int64((Lon + 180.0) / 360.0 * double(TileCount));
        OutY = int64((1.0 - FMath::Loge(FMath::Tan(LatRad) + 1.0 / FMath::Cos(LatRad)) / PI) / 2.0 * double(TileCount));
        OutX = FMath::Clamp(OutX, int64(0), TileCount - 1);
        OutY = FMath::Clamp(OutY, int64(0), TileCount - 1);
    }

    /**
     * @brief Interleaves tile X/Y bits into a quadkey integer.
     *
     * Each zoom level adds two bits (the Y bit above the X bit), coarsest level first, so the
     * key of a parent tile is a prefix of its children's keys.
     *
     * @note The loop reads bits Zoom down to 1 of X and Y, not Zoom - 1 down to 0 as in the
     *       usual Bing quadkey definition, so bit 0 of each index is ignored. Tile keys and
     *       bounds in this project are all built through this function and stay consistent
     *       with each other. Do not change it without re-checking the result against the
     *       `attributes/geotile` values stored in Ditto.
     *
     * @param X     Tile column at Zoom.
     * @param Y     Tile row at Zoom.
     * @param Zoom  Zoom level, 0 to 31 (the key needs 2 * Zoom bits).
     * @return      The quadkey as an integer; 0 when Zoom is 0.
     */
    static int64 TileXYToQuadkey(int64 X, int64 Y, int32 Zoom)
    {
        int64 Key = 0;
        for (int32 I = Zoom; I > 0; --I)
            Key = (Key << 2) | (((Y >> I) & 1) << 1) | ((X >> I) & 1);
        return Key;
    }

    /**
     * @brief Returns the geotile (quadkey integer) of the tile that contains a geographic point.
     *
     * @param Lat   Latitude in decimal degrees.
     * @param Lon   Longitude in decimal degrees.
     * @param Zoom  Zoom level, 0 to 31. Defaults to 18, the precision TRACI vehicles use for
     *              `attributes/geotile` (checked against a live "traci" vehicle).
     * @return      The geotile value, in the same encoding as TileXYToQuadkey().
     */
    static int64 LatLonToGeotile(double Lat, double Lon, int32 Zoom = 18)
    {
        int64 X, Y;
        LatLonToTileXY(Lat, Lon, Zoom, X, Y);
        return TileXYToQuadkey(X, Y, Zoom);
    }

    /**
     * @brief Computes the range of stored geotile values that fall inside the tile containing
     *        a geographic point.
     *
     * Because a parent key is a prefix of its children's keys, every MaxZoom geotile inside the
     * TileZoom tile lies in [OutLower, OutUpper). Use it to build a
     * `ge(attributes/geotile,Lower)` / `lt(attributes/geotile,Upper)` search filter.
     *
     * @param Lat       Latitude in decimal degrees.
     * @param Lon       Longitude in decimal degrees.
     * @param TileZoom  Zoom level of the query tile (higher = smaller tile).
     * @param OutLower  Receives the inclusive lower bound.
     * @param OutUpper  Receives the exclusive upper bound.
     * @param MaxZoom   Zoom level at which the geotile values were stored. Defaults to 18.
     *                  When TileZoom is finer than this, the range collapses to the single
     *                  MaxZoom tile that contains the point.
     */
    static void GeotileBounds(double Lat, double Lon, int32 TileZoom,
                               int64& OutLower, int64& OutUpper, int32 MaxZoom = 18)
    {
        const int64 Key = LatLonToGeotile(Lat, Lon, TileZoom);
        if (TileZoom <= MaxZoom)
        {
            const int32 Shift = 2 * (MaxZoom - TileZoom);
            OutLower = Key << Shift;
            OutUpper = (Key + 1) << Shift;
        }
        else
        {
            // Requested tile is finer than the zoom level Ditto stores geotiles at. Collapse
            // down to the single MaxZoom-precision tile that contains it, since no query can
            // narrow past the data's own precision.
            const int32 Shift = 2 * (TileZoom - MaxZoom);
            const int64 CollapsedKey = Key >> Shift;
            OutLower = CollapsedKey;
            OutUpper = CollapsedKey + 1;
        }
    }
};
