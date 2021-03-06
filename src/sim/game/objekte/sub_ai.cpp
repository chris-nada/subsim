#include "sub_ai.hpp"
#include "../../welt.hpp"
#include "../../physik.hpp"

#include <nada/random.hpp>

Sub_AI::Sub_AI(const Sub& sub) : Sub(sub), timer(0), timer_next_action(AI_THINK_INTERVALL) {

}

bool Sub_AI::tick(Welt* welt, float s) {
    if (const bool alive = Sub::tick(welt, s); !alive) return false; // Lebt nicht mehr

    // Nicht jeden tick() nachdenken
    timer += s;
    if (timer < timer_next_action) return true;
    timer = 0; timer_next_action = AI_THINK_INTERVALL; // Timer zurücksetzen

    // TODO: Statuslogik/übergänge prüfen

    // Wenn unter Beschuss, sofort reagieren
    maybe_evade(welt);

    // Verschossene Torpedos aktualisieren
    update_ziele_torpedos(welt);

    // Kontakte sammeln
    std::vector<const Detektion*> d_active; // Nur AS
    std::vector<const Detektion*> d_passive; // Nur PS
    get_valide_ziele(welt, d_active, d_passive);
    std::vector<const Detektion*> detektionen(d_active); // AS + PS
    detektionen.insert(detektionen.end(), d_passive.begin(), d_passive.end());

    // Kontakte? - Evtl. Kontakt angreifen
    if (detektionen.empty()) remove_status(HIDE);
    else maybe_attack(welt, detektionen, d_active, d_passive);

    // Nichts zu tun -> Zone einnehmen / bewachen
    if (status == DONE) chose_new_job(welt);

    // Tiefe
    if (hat_status(HIDE)) set_target_depth(-500);
    else set_target_depth(-50);

    // Zum Ziel bewegen
    if (hat_status(MOVE)) {
        // Ziel erreicht?
        if (Physik::distanz(pos.x(), pos.y(), ziel.x(), ziel.y()) <= TARGET_DISTANCE) {
            nada::Log::debug() << "Sub_AI " << id << " target reached" << nada::Log::endl;
            remove_status(MOVE);
            stop();
        }
        set_target_pos(ziel.x(), ziel.y());
        set_target_depth(DEPTH_TRAVEL);
        set_target_v(SPEED_TRAVEL);
    } else set_target_v(0);
    return true;
}

void Sub_AI::update_ziele_torpedos(const Welt* welt) {
    std::vector<oid_t> ziele_loeschen;    // <- nicht mehr existierende Ziele
    std::vector<oid_t> torpedos_loeschen; // <- nicht mehr existierende Torpedos
    for (auto& [ziel_id, torpedo_ids] : ziele_torpedos) {
        for (auto torpedo_id : torpedo_ids) if (const Objekt* o = welt->get_objekt_or_null(torpedo_id); !o) torpedos_loeschen.push_back(torpedo_id);
        if (const Objekt* o = welt->get_objekt_or_null(ziel_id); !o) ziele_loeschen.push_back(ziel_id);
    }
    // Gesammelte Objekte loeschen
    for (const oid_t ziel_id : ziele_loeschen) ziele_torpedos.erase(ziel_id);
    for (const oid_t t_id : torpedos_loeschen) for (auto& [ziel_id, t_set] : ziele_torpedos) t_set.erase(t_id);

    // Leere sets löschen
    for (auto it = ziele_torpedos.begin(); it != ziele_torpedos.end();) {
        if (it->second.empty()) it = ziele_torpedos.erase(it);
        else ++it;
    }
}

void Sub_AI::get_valide_ziele(const Welt* welt, std::vector<const Detektion*>& d_active, std::vector<const Detektion*>& d_passive) {
    auto is_angriffsziel = [&](const Detektion& d) {
        if (ziele_torpedos.count(d.objekt_id) != 0 && !ziele_torpedos[d.objekt_id].empty()) return false; // bereits unter Beschuss
        const Objekt* obj = welt->get_objekt_or_null(d.objekt_id);
        if (obj == nullptr) return false; // zwischenzeitlich zerstört
        return (obj->get_typ() == Objekt::Typ::SUB || obj->get_typ() == Objekt::Typ::SUB_AI) && obj->get_team() != this->get_team();
    };
    for (const auto& as: get_sonars_active())   for (const auto& d: as.get_detektionen()) if (is_angriffsziel(d)) d_active.push_back(&d);
    for (const auto& ps: get_sonars_passive()) for (const auto& d: ps.get_detektionen()) if (is_angriffsziel(d)) d_passive.push_back(&d);
    std::vector<const Detektion*> detektionen(d_active); // AS + PS
    detektionen.insert(detektionen.end(), d_passive.begin(), d_passive.end());
}

void Sub_AI::maybe_evade(Welt* welt) {
    for (const auto& [id, o] : welt->objekte) {
        // Feindliches Torpedo?
        if (o->get_typ() == Objekt::Typ::TORPEDO && o->get_team() != this->team) {
            // TODO check ob Torpedo gesehen wird sonst ist das Botverhalten evtl. schummelig

            // Ist es nah?
            const auto d = Physik::distanz_xyz(o->get_pos(), this->pos);
            if (d > EVADE_DISTANCE) continue;

            // Ausweichmanöver
            set_target_v(1.f);
            set_target_depth(nada::random::f(-500.f, -150.f));
            if (get_bearing() < 180.f) set_target_bearing(kurs + 160.f);
            else set_target_bearing(kurs - 160.f);

            // Decoy falls vorhanden
            const auto decoy_it = std::find_if(decoys.begin(), decoys.end(), [](const auto& paar) { return paar.second > 0; });
            if (decoy_it != decoys.end()) deploy_decoy(welt, decoy_it->first.get_name());
            timer_next_action = 60.f;
            return;
        }
    }
}

void Sub_AI::maybe_attack(Welt* welt, const std::vector<const Detektion*>& detektionen, const std::vector<const Detektion*>& d_active, const std::vector<const Detektion*>& d_passive) {
    // AS Detektion? Ja - Hin/Angreifen; Nein - Suchen
    if (d_active.empty() && !d_passive.empty()) add_status(SEARCH); // !AS, aber PS -> Suchen
    else if (!d_active.empty()) remove_status(SEARCH); // AS Kontakt vorhanden -> nicht mehr suchen

    // Mit AS suchen?
    if (d_active.empty() && !sonars_active.empty() && !hat_status(HIDE) && hat_status(SEARCH)) {
        nada::Log::debug() << "Sub_AI " << id << " trying single Ping" << nada::Log::endl;
        auto& as = sonars_active[0];
        as.set_ping_intervall(300); // zu häufiges Pingen verhindern
        as.set_mode(Sonar_Aktiv::Mode::SINGLE);
    }

    // Zielauswahl
    const auto torpedo_range = this->get_max_reichweite_torpedo(true);
    const Detektion* angriffsziel = detektionen[0];
    for (const auto& d : detektionen) {
        if (angriffsziel->range.has_value() && d->range.has_value() && d->range < angriffsziel->range) angriffsziel = d;
    }

    // Kein AS Kontakt oder Ziel außerhalb Torpedoreichweite
    if (!angriffsziel->range.has_value() || (angriffsziel->range.has_value() && angriffsziel->range.value() > torpedo_range)) {
        const auto [ziel_x, ziel_y] = Physik::get_punkt(this->pos.x(), this->pos.y(), angriffsziel->bearing, 1'000);
        this->ziel.x(ziel_x); this->ziel.y(ziel_y);
        remove_status(SEARCH);
        add_status(MOVE);
    }
    else { // Angriff!
        std::optional<Torpedo> torpedo_wahl; // Torpedo auswählen
        for (const auto& [torpedo, muni] : this->torpedos) {
            if (muni && torpedo.get_range() > angriffsziel->range.value()) {
                // Zufälliges Torpedo mit genügend Reichweite
                if (!torpedo_wahl.has_value() || nada::random::b(50)) torpedo_wahl = torpedo;
            }
        }
        // Torpedo einstellen ?
        if (torpedo_wahl.has_value()) {
            const Torpedo t(
                    torpedo_wahl.value(),
                    this,
                    angriffsziel->range.value() * 0.5,
                    angriffsziel->bearing
            );
            // Feuer!
            const auto& torp_id_opt = welt->add_torpedo(this, &t);
            if (torp_id_opt) ziele_torpedos[angriffsziel->objekt_id].insert(torp_id_opt.value());
        }
    }
}

void Sub_AI::chose_new_job(const Welt* welt) {
    nada::Log::debug() << "Sub_AI " << id << " looking for new job" << nada::Log::endl;

    // Zone finden und als Ziel setzen
    const auto it = std::find_if(welt->zonen.begin(), welt->zonen.end(), [this](const Zone& zone) {
        return zone.get_team() != this->get_team();
    });
    // Feindlicher Zone einnehmen
    if (it != welt->zonen.end()) ziel = Vektor(std::get<0>(it->get_pos()), std::get<1>(it->get_pos()), DEPTH_TRAVEL);
    else {
        // Zu zufälliger Zone reisen
        const Zone& zone = welt->zonen[nada::random::ui(0, welt->zonen.size()-1)];
        ziel = Vektor(std::get<0>(zone.get_pos()),  std::get<1>(zone.get_pos()), DEPTH_TRAVEL);
    }
    add_status(MOVE);
}
