#include <Ogre.h>
#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>

/// Prädeklaration.
class Welt;

/// Elternklasse für alle von der Physiksimulation betroffenen Objekte.
class Objekt {

public:

    /// Vererbungshierachie von Objekt. Zur Typenbestimmung bei Laufzeit.
    enum class Typ { OBJEKT, OBJEKT_STEUERBAR, SUB, TORPEDO };

    /// Ctor.
    Objekt() = default;

    /// Ctor.
    explicit Objekt(const Ogre::Vector3& pos, const Ogre::Quaternion& orientation =
                    Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y));

    /// Dtor.
    virtual ~Objekt();

    /// Führt einen Simulationstick für Zeit `s` aus.
    virtual void tick(Welt* welt, float s) {}

    /// Liefert den Objekttypen zur Polymorphieauflösung.
    virtual Typ get_typ() const { return Typ::OBJEKT; }

    /// Getter: Einmalige ID, global gültig für alle Objekte und vererbte Klassen.
    uint32_t get_id() const { return id; }

    /// Getter: Teamzugehörigkeit. 0 = Kein Team.
    uint8_t get_team() const { return team; }

    /// Getter: Position.
    const Ogre::Vector3& get_pos() const { return pos; }

    /// Getter: Orientierung.
    const Ogre::Quaternion& get_orientation() const { return orientation; }

    /// Getter: 0° bis 360° Ausrichtung.
    float get_bearing() const;

    /// Getter: -90° bis 90° Steigung.
    float get_pitch() const;

    /// Serialisierung via cereal.
    template <class Archive> void serialize(Archive& ar) {
        ar(id, team, pos.x, pos.y, pos.z, orientation.x, orientation.y, orientation.z, orientation.w);
    }

protected:

    /// Einmalige ID. Wird nicht aufsteigend generiert, sondern zufällig.
    uint32_t id;

    /// Teamzugehörigkeit. 0 = Kein Team.
    uint8_t team = 0;

    /// Position.
    Ogre::Vector3 pos;

    /// Ausrichtung.
    Ogre::Quaternion orientation;

private:

    /// Vergebene IDs.
    static inline std::unordered_set<uint32_t> given_ids;

};