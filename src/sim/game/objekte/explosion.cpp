#include <log.hpp>
#include "explosion.hpp"
#include "../../welt.hpp"
#include "../../physik.hpp"

Explosion::Explosion(dist_t radius, float power, float remaining_time,
                     const Vektor& pos, float bearing, oid_t source)
        : Objekt(pos, bearing), quelle(source), radius(radius), power(power), remaining_time(remaining_time)
{

}

Explosion::Explosion(const Torpedo* torpedo) : Explosion(torpedo->explosion) {
    pos    = torpedo->pos;
    quelle = torpedo->quelle;
    kurs   = 0;
    pitch  = 0;
}

bool Explosion::tick(Welt* welt, float s) {
    /// Objekten Schaden zufügen
    if (!damage_done) {
        for (auto& paar : welt->get_objekte()) {
            // Gültiges Zielobjekt?
            Objekt* o = paar.second.get();
            if (this == o) continue;
            if (o->get_typ() == Typ::EXPLOSION) continue;
            if (!Physik::in_reichweite_xyz(this->get_pos(), o->get_pos(), radius)) continue;

            // Schaden anwenden
            const double d = Physik::distanz_xyz(this->get_pos(), o->get_pos());
            const double range_faktor = 1.0 - (d / radius);
            const float damage = range_faktor * power;
            Log::debug() << "Explosion beschaedigt Objekt " << o->get_id() << " Typ=" << (int)o->get_typ() << " Schaden=" << damage << '\n';
            o->apply_damage(this, damage);
        }
        damage_done = true;
    }

    /// Explosion noch Sichtbar?
    remaining_time -= s;
    return remaining_time > 0;
}

