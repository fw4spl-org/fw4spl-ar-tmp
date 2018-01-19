/* ***** BEGIN LICENSE BLOCK *****
 * FW4SPL - Copyright (C) IRCAD, 2014-2017.
 * Distributed under the terms of the GNU Lesser General Public License (LGPL) as
 * published by the Free Software Foundation.
 * ****** END LICENSE BLOCK ****** */

#ifndef __VIDEOCALIBRATION_SINTRINSIC_HPP__
#define __VIDEOCALIBRATION_SINTRINSIC_HPP__

#include "videoCalibration/config.hpp"
#include "videoCalibration/ICalibration.hpp"

namespace videoCalibration
{
/**
 * @brief   SIntrinsic service that computes intrinsic calibration.
 */
class VIDEOCALIBRATION_CLASS_API SIntrinsic : public ::videoCalibration::ICalibration
{
public:
    fwCoreServiceClassDefinitionsMacro((SIntrinsic)(::videoCalibration::ICalibration));

    /// Constructor.
    VIDEOCALIBRATION_API SIntrinsic() noexcept;

    /// Destructor.
    VIDEOCALIBRATION_API virtual ~SIntrinsic() noexcept;

protected:

    /// Does nothing
    /**
     * @brief Configures the service.
     * @code{.xml}
       <service impl="::videoCalibration::SIntrinsic" >
            <calibrationInfoID> ... </calibrationInfoID>
            <board width="17" height="13" />
       </service>
       @endcode
     * - \b calibrationInfoID: FwId of calibrationInfo.
     * - \b board : defines the number of square in 2 dimensions of the chessboard.
     */
    VIDEOCALIBRATION_API void configuring() override;

    /// Does nothing.
    VIDEOCALIBRATION_API void starting() override;

    /// Calls stopping and starting.
    VIDEOCALIBRATION_API void swapping() override;

    /// Computes intrinsic calibration
    VIDEOCALIBRATION_API void updating() override;

    /// Removes connections
    VIDEOCALIBRATION_API void stopping() override;

private:

    ///  FwId of calibrationInfo
    std::string m_calibrationInfoID;

    /// Width of the chessboard used for calibration
    unsigned int m_width;

    /// Height of the chessboard used for calibration
    unsigned int m_height;

};
} // namespace videoCalibration

#endif // __VIDEOCALIBRATION_SINTRINSIC_HPP__