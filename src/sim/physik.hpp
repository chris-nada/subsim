#pragma once

#include <array>

#include "vektor.hpp"
#include "typedefs.hpp"

class Objekt;

/// Mathematische/Physikalische Hilfsfunktionen.
namespace Physik {

    /// Rundet `wert` auf `faktor`. (18, 5) ergibt bspw. 20.
    template<typename T> T round(T wert, T faktor);

    /// Bewegt `pos` in Richtung `q` um `amount`.
    void move(Vektor& pos, winkel_t kurs, dist_t weite);
    /// Liefert den Punkt x,y als `std::pair` vom gegebenen Punkt aus gesehen im gg. Winkel bei gg. Entfernung.
    std::pair<dist_t, dist_t> get_punkt(dist_t x, dist_t y, winkel_t kurs, dist_t entfernung);

    /// Liefert den Kurs von Punkt (x,y) zu Punkt (target_x,target_y).
    winkel_t kurs(dist_t x, dist_t y, dist_t target_x, dist_t target_y);
    /// Liefert den Kurs von Punkt (x,y) zu Punkt (target_x,target_y).
    winkel_t kurs(const Vektor& v, const Vektor& v_target);
    /// Liefert den relativen Kurs von Objekt1 zu Objekt2; also in welcher Richtung liegt o2 von o1 aus gesehen? [-180 bis +180].
    winkel_t kurs_relativ(const Objekt* o1, const Objekt* o2);

    /// Liefert die Distanz zwischen zwei 2D-Koordinaten.
    dist_t distanz(dist_t x1, dist_t y1, dist_t x2, dist_t y2);
    /// Liefert die xy-Distanz zwischen zwei 3D-Koordinaten.
    dist_t distanz_xy(const Vektor& v1, const Vektor& v2);
    /// Liefert die xyz-Distanz zwischen zwei 3D-Koordinaten.
    dist_t distanz_xyz(const Vektor& v1, const Vektor& v2);
    /// Liefert distanz(v1,v2) <= reichweite.
    bool in_reichweite_xyz(const Vektor& v1, const Vektor& v2, dist_t reichweite);
    /// Liefert den Bremsweg bei gegebener Geschwindigkeit und Entschleunigung.
    dist_t bremsweg(float v, float a);

    /// Liefert die kürzeste Rotation (in Grad) von Winkel 1 zu Winkel 2. Zwischen -180 und +180.
    winkel_t winkel_diff(winkel_t winkel1, winkel_t winkel2);
    /// Liefert den Winkel in der y-Ebene (d.h. Höhe/Tiefe) zwischen zwei 3D-Punkten.
    winkel_t winkel_tiefe(const Vektor& pos, const Vektor& target_pos);
    /// Normalisiert gegebenen Winkel, sodass -180 <= winkel <= +180. // TODO test? / deprecated?
    [[deprecated]] winkel_t winkel_norm(winkel_t winkel);
    /// Prüft, ob `test_winkel` maximal `winkelbereich` Grad (°) abweicht von winkel.
    bool is_winkel_im_bereich(winkel_t test_winkel, winkel_t ziel_winkel, winkel_t bereich);

    /**
     * Liefert die Geräuschverringung nach Distanz (0.0 - kein Schall übrig - 1.0).
     * @note Die verwendete Formel lautet: `1 / (1 + 0.000000002 * d²)`.
     */
    float schallfaktor(dist_t distanz);

    /**
     * Liefert die "Sichtbarkeit" (also Geräuschentwicklung) eines Subs von 0.0 (unsichtbar) - 1.0 (ganz deutlich).
     * @param sichtbarkeitsfaktor 0.0 - 1.0 (ja nach Technologie; kleiner ist besser).
     * @param v Geschwindigkeit
     * @param d Entfernung
     * @note Verhältnismäßig teuer. Sollte nicht jeden Tick für jedes Objekt berechnet werden.
     * @note Bei sehr kleinen v (<1/128) wird der halbe Wert zurückgegeben, wodurch "Motor aus" simuliert wird.
     * @return 0.1 und weniger bedeutet quasi unsichtbar. Absolut 0 wird praktisch nie erreicht.
     */
    float sichtbarkeit(float sichtbarkeitsfaktor, float v, dist_t d);

}
