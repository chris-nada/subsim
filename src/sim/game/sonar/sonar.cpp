#include "sonar.hpp"
#include "../../physik.hpp"

Sonar::Sonar(const std::string& name, Groesse groesse, float resolution, std::vector<std::tuple<float, float>> blindspots) :
    name(name),
    resolution(resolution),
    timer(0),
    groesse(groesse),
    blindspots(std::move(blindspots))
{
    //
}

bool Sonar::is_in_toter_winkel(winkel_t kurs_relativ) const {
    return std::any_of(blindspots.begin(), blindspots.end(), [&](const auto& blindspot) {
        return Physik::is_winkel_im_bereich(kurs_relativ, std::get<0>(blindspot), std::get<1>(blindspot));
    });
}
