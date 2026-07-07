#pragma once

#include "CoreMinimal.h"

/**
 * Lightweight geotile / OSM quadkey utilities.
 * No UObject dependency — safe to include from any struct or service header.
 *
 * Geotiles are 64-bit quadkey integers computed at a given zoom level using
 * the Web Mercator (EPSG:3857) tile scheme. Ditto stores them at zoom 31.
 *
 * These functions match UDittoService::GetQuadkey / GetTileXY exactly.
 */
struct FGeotileUtils
{
    /** Converts a geographic point to OSM tile X/Y at the given zoom level. */
    static void LatLonToTileXY(double Lat, double Lon, int32 Zoom, int64& OutX, int64& OutY)
    {
        const int64  TileCount = int64(1) << Zoom;
        const double LatRad    = FMath::DegreesToRadians(Lat);
        OutX = int64((Lon + 180.0) / 360.0 * double(TileCount));
        OutY = int64((1.0 - FMath::Loge(FMath::Tan(LatRad) + 1.0 / FMath::Cos(LatRad)) / PI) / 2.0 * double(TileCount));
        OutX = FMath::Clamp(OutX, int64(0), TileCount - 1);
        OutY = FMath::Clamp(OutY, int64(0), TileCount - 1);
    }

    /** Interleaves tile X/Y bits into a quadkey integer at the given zoom level. */
    static int64 TileXYToQuadkey(int64 X, int64 Y, int32 Zoom)
    {
        int64 Key = 0;
        for (int32 I = Zoom; I > 0; --I)
            Key = (Key << 2) | (((Y >> I) & 1) << 1) | ((X >> I) & 1);
        return Key;
    }

    /**
     * Returns the OSM quadkey integer for a geographic point at the given zoom level.
     * Default zoom is 31 — the level at which Ditto stores geotiles.
     */
    static int64 LatLonToGeotile(double Lat, double Lon, int32 Zoom = 31)
    {
        int64 X, Y;
        LatLonToTileXY(Lat, Lon, Zoom, X, Y);
        return TileXYToQuadkey(X, Y, Zoom);
    }

    /**
     * Returns the inclusive lower / exclusive upper geotile range for the tile
     * containing a geographic point at TileZoom, stored at MaxZoom precision.
     */
    static void GeotileBounds(double Lat, double Lon, int32 TileZoom,
                               int64& OutLower, int64& OutUpper, int32 MaxZoom = 31)
    {
        const int64 Key  = LatLonToGeotile(Lat, Lon, TileZoom);
        const int32 Shift = 2 * (MaxZoom - TileZoom);
        OutLower = Key << Shift;
        OutUpper = (Key + 1) << Shift;
    }
};
