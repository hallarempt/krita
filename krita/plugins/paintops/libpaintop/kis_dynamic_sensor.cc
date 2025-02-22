/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2011 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_dynamic_sensor.h"
#include <QDomElement>

#include "kis_algebra_2d.h"

#include "sensors/kis_dynamic_sensors.h"
#include "sensors/kis_dynamic_sensor_distance.h"
#include "sensors/kis_dynamic_sensor_drawing_angle.h"
#include "sensors/kis_dynamic_sensor_time.h"
#include "sensors/kis_dynamic_sensor_fade.h"
#include "sensors/kis_dynamic_sensor_fuzzy.h"

KisDynamicSensor::KisDynamicSensor(DynamicSensorType type)
    : m_length(-1)
    , m_type(type)
    , m_customCurve(false)
    , m_active(false)
{
}

KisDynamicSensor::~KisDynamicSensor()
{
}

QWidget* KisDynamicSensor::createConfigurationWidget(QWidget* parent, QWidget*)
{
    Q_UNUSED(parent);
    return 0;
}

void KisDynamicSensor::reset()
{
}

KisDynamicSensorSP KisDynamicSensor::id2Sensor(const KoID& id)
{
    if (id.id() == PressureId.id()) {
        return new KisDynamicSensorPressure();
    }
    else if (id.id() == PressureInId.id()) {
        return new KisDynamicSensorPressureIn();
    }
    else if (id.id() == XTiltId.id()) {
        return new KisDynamicSensorXTilt();
    }
    else if (id.id() == YTiltId.id()) {
        return new KisDynamicSensorYTilt();
    }
    else if (id.id() == TiltDirectionId.id()) {
        return new KisDynamicSensorTiltDirection();
    }
    else if (id.id() == TiltElevationId.id()) {
        return new KisDynamicSensorTiltElevation();
    }
    else if (id.id() == SpeedId.id()) {
        return new KisDynamicSensorSpeed();
    }
    else if (id.id() == DrawingAngleId.id()) {
        return new KisDynamicSensorDrawingAngle();
    }
    else if (id.id() == RotationId.id()) {
        return new KisDynamicSensorRotation();
    }
    else if (id.id() == DistanceId.id()) {
        return new KisDynamicSensorDistance();
    }
    else if (id.id() == TimeId.id()) {
        return new KisDynamicSensorTime();
    }
    else if (id.id() == FuzzyId.id()) {
        return new KisDynamicSensorFuzzy();
    }
    else if (id.id() == FadeId.id()) {
        return new KisDynamicSensorFade();
    }
    else if (id.id() == PerspectiveId.id()) {
        return new KisDynamicSensorPerspective();
    }
    else if (id.id() == TangentialPressureId.id()) {
        return new KisDynamicSensorTangentialPressure();
    }
    dbgPlugins << "Unknown transform parameter :" << id.id();
    return 0;
}

DynamicSensorType KisDynamicSensor::id2Type(const KoID &id)
{
    if (id.id() == PressureId.id()) {
        return PRESSURE;
    }
    else if (id.id() == PressureInId.id()) {
        return PRESSURE_IN;
    }
    else if (id.id() == XTiltId.id()) {
        return XTILT;
    }
    else if (id.id() == YTiltId.id()) {
        return YTILT;
    }
    else if (id.id() == TiltDirectionId.id()) {
        return TILT_DIRECTION;
    }
    else if (id.id() == TiltElevationId.id()) {
        return TILT_ELEVATATION;
    }
    else if (id.id() == SpeedId.id()) {
        return SPEED;
    }
    else if (id.id() == DrawingAngleId.id()) {
        return ANGLE;
    }
    else if (id.id() == RotationId.id()) {
        return ROTATION;
    }
    else if (id.id() == DistanceId.id()) {
        return DISTANCE;
    }
    else if (id.id() == TimeId.id()) {
        return TIME;
    }
    else if (id.id() == FuzzyId.id()) {
        return FUZZY;
    }
    else if (id.id() == FadeId.id()) {
        return FADE;
    }
    else if (id.id() == PerspectiveId.id()) {
        return PERSPECTIVE;
    }
    else if (id.id() == TangentialPressureId.id()) {
        return TANGENTIAL_PRESSURE;
    }
    return UNKNOWN;
}

KisDynamicSensorSP KisDynamicSensor::type2Sensor(DynamicSensorType sensorType)
{
    switch (sensorType) {
    case FUZZY:
        return new KisDynamicSensorFuzzy();
    case SPEED:
        return new KisDynamicSensorSpeed();
    case FADE:
        return new KisDynamicSensorFade();
    case DISTANCE:
        return new KisDynamicSensorDistance();
    case TIME:
        return new KisDynamicSensorTime();
    case ANGLE:
        return new KisDynamicSensorDrawingAngle();
    case ROTATION:
        return new KisDynamicSensorRotation();
    case PRESSURE:
        return new KisDynamicSensorPressure();
    case XTILT:
        return new KisDynamicSensorXTilt();
    case YTILT:
        return new KisDynamicSensorYTilt();
    case TILT_DIRECTION:
        return new KisDynamicSensorTiltDirection();
    case TILT_ELEVATATION:
        return new KisDynamicSensorTiltElevation();
    case PERSPECTIVE:
        return new KisDynamicSensorPerspective();
    case TANGENTIAL_PRESSURE:
        return new KisDynamicSensorTangentialPressure();
    case PRESSURE_IN:
        return new KisDynamicSensorPressureIn();
    default:
        return 0;
    }
}

QString KisDynamicSensor::minimumLabel(DynamicSensorType sensorType)
{
    switch (sensorType) {
    case FUZZY:
        return QString();
    case FADE:
        return i18n("0");
    case DISTANCE:
        return i18n("0 px");
    case TIME:
        return i18n("0 s");
    case ANGLE:
        return i18n("0°");
    case SPEED:
        return i18n("Slow");
    case ROTATION:
        return i18n("0°");
    case PRESSURE:
        return i18n("Low");
    case XTILT:
        return i18n("-30°");
    case YTILT:
        return i18n("-30°");
    case TILT_DIRECTION:
        return i18n("0°");
    case TILT_ELEVATATION:
        return i18n("90°");
    case PERSPECTIVE:
        return i18n("Far");
    case TANGENTIAL_PRESSURE:
    case PRESSURE_IN:
        return i18n("Low");
    default:
        return i18n("0.0");
    }
}

QString KisDynamicSensor::maximumLabel(DynamicSensorType sensorType, int max)
{
    switch (sensorType) {
    case FUZZY:
        return QString();
    case FADE:
        if (max < 0)
            return i18n("1000");
        else
            return i18n("%1", max);
    case DISTANCE:
        if (max < 0)
            return i18n("30 px");
        else
            return i18n("%1 px", max);
    case TIME:
        if (max < 0)
           return i18n("3 s");
        else
            return i18n("%1 s", max / 1000);
    case ANGLE:
        return i18n("360°");
    case SPEED:
        return i18n("Fast");
    case ROTATION:
        return i18n("360°");
    case PRESSURE:
        return i18n("High");
    case XTILT:
        return i18n("30°");
    case YTILT:
        return i18n("30°");
    case TILT_DIRECTION:
        return i18n("360°");
    case TILT_ELEVATATION:
        return i18n("0°");
    case PERSPECTIVE:
        return i18n("Near");
    case TANGENTIAL_PRESSURE:
    case PRESSURE_IN:        
        return i18n("High");
    default:
        return i18n("1.0");
    };
}


KisDynamicSensorSP KisDynamicSensor::createFromXML(const QString& s)
{
    QDomDocument doc;
    doc.setContent(s);
    QDomElement e = doc.documentElement();
    return createFromXML(e);
}

KisDynamicSensorSP KisDynamicSensor::createFromXML(const QDomElement& e)
{
    QString id = e.attribute("id", "");
    KisDynamicSensorSP sensor = id2Sensor(id);
    if (sensor) {
        sensor->fromXML(e);
    }
    return sensor;
}

QList<KoID> KisDynamicSensor::sensorsIds()
{
    QList<KoID> ids;

    ids << PressureId
        << PressureInId
        << XTiltId
        << YTiltId
        << TiltDirectionId
        << TiltElevationId
        << SpeedId
        << DrawingAngleId
        << RotationId
        << DistanceId
        << TimeId
        << FuzzyId
        << FadeId
        << PerspectiveId
        << TangentialPressureId;

    return ids;
}

QList<DynamicSensorType> KisDynamicSensor::sensorsTypes()
{
    QList<DynamicSensorType> sensorTypes;
    sensorTypes
            << PRESSURE
            << PRESSURE_IN
            << XTILT
            << YTILT
            << TILT_DIRECTION
            << TILT_ELEVATATION
            << SPEED
            << ANGLE
            << ROTATION
            << DISTANCE
            << TIME
            << FUZZY
            << FADE
            << PERSPECTIVE
            << TANGENTIAL_PRESSURE;
    return sensorTypes;
}

QString KisDynamicSensor::id(DynamicSensorType sensorType)
{
    switch (sensorType) {
    case FUZZY:
        return "fuzzy";
    case FADE:
        return "fade";
    case DISTANCE:
        return "distance";
    case TIME:
        return "time";
    case ANGLE:
        return "drawingangle";
    case SPEED:
        return "speed";
    case ROTATION:
        return "rotation";
    case PRESSURE:
        return "pressure";
    case XTILT:
        return "xtilt";
    case YTILT:
        return "ytilt";
    case TILT_DIRECTION:
        return "ascension";
    case TILT_ELEVATATION:
        return "declination";
    case PERSPECTIVE:
        return "perspective";
    case TANGENTIAL_PRESSURE:
        return "tangentialpressure";
    case PRESSURE_IN:
        return "pressurein";
    case SENSORS_LIST:
        return "sensorslist";
    default:
        return QString();
    };
}


void KisDynamicSensor::toXML(QDomDocument& doc, QDomElement& elt) const
{
    elt.setAttribute("id", id(sensorType()));
    if (m_customCurve) {
        QDomElement curve_elt = doc.createElement("curve");
        QDomText text = doc.createTextNode(m_curve.toString());
        curve_elt.appendChild(text);
        elt.appendChild(curve_elt);
    }
}

void KisDynamicSensor::fromXML(const QDomElement& e)
{
    Q_UNUSED(e);
    Q_ASSERT(e.attribute("id", "") == id(sensorType()));
    m_customCurve = false;
    QDomElement curve_elt = e.firstChildElement("curve");
    if (!curve_elt.isNull()) {
        m_customCurve = true;
        m_curve.fromString(curve_elt.text());
    }
}

qreal KisDynamicSensor::parameter(const KisPaintInformation& info)
{
    qreal val = value(info);
    if (m_customCurve) {
        int offset = qRound(256.0 * qAbs(val));
        qreal newValue =  m_curve.floatTransfer(257)[qBound(0, offset, 256)];
        return KisAlgebra2D::copysign(newValue, val);
    }
    else {
        return val;
    }
}

void KisDynamicSensor::setCurve(const KisCubicCurve& curve)
{
    m_customCurve = true;
    m_curve = curve;
}

const KisCubicCurve& KisDynamicSensor::curve() const
{
    return m_curve;
}

void KisDynamicSensor::removeCurve()
{
    m_customCurve = false;
}

bool KisDynamicSensor::hasCustomCurve() const
{
    return m_customCurve;
}

bool KisDynamicSensor::dependsOnCanvasRotation() const
{
    return true;
}

bool KisDynamicSensor::isAdditive() const
{
    return false;
}

void KisDynamicSensor::setActive(bool active)
{
    m_active = active;
}

bool KisDynamicSensor::isActive() const
{
    return m_active;
}
