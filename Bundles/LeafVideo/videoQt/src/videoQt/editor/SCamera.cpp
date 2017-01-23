/* ***** BEGIN LICENSE BLOCK *****
 * FW4SPL - Copyright (C) IRCAD, 2014-2017.
 * Distributed under the terms of the GNU Lesser General Public License (LGPL) as
 * published by the Free Software Foundation.
 * ****** END LICENSE BLOCK ****** */

#include "videoQt/editor/SCamera.hpp"

#include "videoQt/editor/CameraDeviceDlg.hpp"

#include <arData/CameraSeries.hpp>

#include <arPreferences/preferences.hpp>

#include <QByteArray>
#include <QCamera>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <fwCom/Signal.hxx>

#include <fwData/Object.hpp>
#include <fwData/location/Folder.hpp>
#include <fwData/location/SingleFile.hpp>

#include <fwGui/dialog/InputDialog.hpp>
#include <fwGui/dialog/LocationDialog.hpp>

#include <fwGuiQt/container/QtContainer.hpp>

#include <fwRuntime/ConfigurationElement.hpp>
#include <fwRuntime/operations.hpp>

#include <fwServices/macros.hpp>

#include <fwTools/pathDifference.hpp>

namespace videoQt
{
namespace editor
{

fwServicesRegisterMacro( ::gui::editor::IEditor, ::videoQt::editor::SCamera );

//------------------------------------------------------------------------------

SCamera::SCamera() throw() :
    m_bVideoSupport(false),
    m_numCreateCameras(0)
{
}

//------------------------------------------------------------------------------

SCamera::~SCamera() throw()
{
}

//------------------------------------------------------------------------------

void SCamera::configuring() throw(fwTools::Failed)
{
    ::fwServices::IService::ConfigType config = this->getConfigTree().get_child("service");

    m_bVideoSupport    = (config.get<std::string>("videoSupport", "no") == "yes");
    m_numCreateCameras = config.get<size_t>("createCameraNumber", 0);

    this->initialize();
}

//------------------------------------------------------------------------------

void SCamera::starting() throw(::fwTools::Failed)
{
    this->create();

    ::fwGuiQt::container::QtContainer::sptr qtContainer = ::fwGuiQt::container::QtContainer::dynamicCast(
        this->getContainer() );
    QWidget* const container = qtContainer->getQtContainer();
    SLM_ASSERT("container not instanced", container);

    QPointer<QHBoxLayout> layout = new QHBoxLayout();
    QPointer<QLabel> sourceLabel = new QLabel(QObject::tr("Video source: "));
    m_devicesComboBox = new QComboBox();
    layout->addWidget(sourceLabel);
    layout->addWidget(m_devicesComboBox);

    m_devicesComboBox->addItem("Device...", "device");

    // Add video file
    if(m_bVideoSupport)
    {
        m_devicesComboBox->addItem("File...", "file");
        m_devicesComboBox->addItem("Stream...", "stream");
    }

    container->setLayout(layout);

    QObject::connect(m_devicesComboBox, SIGNAL(activated(int)), this, SLOT(onApply(int)));

    // Create camera data if necessary
    auto cameraSeries = this->getInOut< ::arData::CameraSeries >("cameraSeries");
    if(cameraSeries)
    {
        const size_t numCameras = cameraSeries->getNumberOfCameras();
        if(numCameras == 0)
        {
            SLM_ASSERT("No camera data in the CameraSeries.", m_numCreateCameras != 0);

            for(size_t i = 0; i < m_numCreateCameras; ++i)
            {
                ::arData::Camera::sptr camera = ::arData::Camera::New();
                cameraSeries->addCamera(camera);
                auto sig = cameraSeries->signal< ::arData::CameraSeries::AddedCameraSignalType >(
                    ::arData::CameraSeries::s_ADDED_CAMERA_SIG);
                sig->asyncEmit(camera);
            }
            OSLM_INFO("No camera data in the CameraSeries, " << m_numCreateCameras << " will be created.");
        }
        else
        {
            SLM_ASSERT("CameraSeries contains camera data but the service is configured to create " <<
                       m_numCreateCameras <<" cameras.", m_numCreateCameras != 0);
        }
    }
}

//------------------------------------------------------------------------------

void SCamera::stopping() throw(::fwTools::Failed)
{
    this->getContainer()->clean();
    this->destroy();
}

//------------------------------------------------------------------------------

void SCamera::updating() throw(::fwTools::Failed)
{
}

//------------------------------------------------------------------------------

void SCamera::swapping() throw(::fwTools::Failed)
{
    this->updating();
}

//------------------------------------------------------------------------------

void SCamera::onApply(int index)
{
    switch(index)
    {
        case 0:
            this->onChooseDevice();
            break;
        case 1:
            this->onChooseFile();
            break;
        case 2:
            this->onChooseStream();
            break;
    }
}

//------------------------------------------------------------------------------

void SCamera::onChooseFile()
{
    std::vector< ::arData::Camera::sptr > cameras = this->getCameras();

    // Check preferences
    const ::boost::filesystem::path videoDirPreferencePath(::arPreferences::getVideoDir());

    static ::boost::filesystem::path _sDefaultPath;

    ::fwGui::dialog::LocationDialog dialogFile;
    dialogFile.setTitle("Choose a file to load a video");
    dialogFile.setDefaultLocation( ::fwData::location::Folder::New(_sDefaultPath) );
    dialogFile.addFilter("mp4", "*.mp4");
    dialogFile.addFilter("avi", "*.avi");
    dialogFile.addFilter("m4v", "*.m4v");
    dialogFile.addFilter("mkv", "*.mkv");
    dialogFile.addFilter("All files", "*.*");
    dialogFile.setOption(::fwGui::dialog::ILocationDialog::READ);
    dialogFile.setOption(::fwGui::dialog::ILocationDialog::FILE_MUST_EXIST);

    for(auto& camera : cameras)
    {
        ::fwData::location::SingleFile::sptr result;
        ::boost::filesystem::path videoPath;
        result = ::fwData::location::SingleFile::dynamicCast( dialogFile.show() );
        if (result)
        {
            _sDefaultPath = result->getPath().parent_path();
            dialogFile.saveDefaultLocation( ::fwData::location::Folder::New(_sDefaultPath) );
            videoPath = result->getPath();
            if(::boost::filesystem::is_directory(videoDirPreferencePath))
            {
                ::boost::filesystem::path videoRelativePath;
                videoRelativePath = ::fwTools::getPathDifference(videoDirPreferencePath, videoPath);

                ::boost::filesystem::path concatenatedPath = videoDirPreferencePath / videoRelativePath;
                if(::boost::filesystem::exists(concatenatedPath))
                {
                    videoPath = videoRelativePath;
                }
                else
                {
                    SLM_WARN("Relative path '"+videoRelativePath.string()+"' genrerated with preference is not valid.");
                }
            }
            else
            {
                SLM_WARN("Video directory '"+videoDirPreferencePath.string()+"' stored in preference is not valid.");
            }
            camera->setCameraSource(::arData::Camera::FILE);
            camera->setVideoFile(videoPath.string());

            ::arData::Camera::ModifiedSignalType::sptr sig;
            sig = camera->signal< ::arData::Camera::ModifiedSignalType >( ::arData::Camera::s_MODIFIED_SIG );
            sig->asyncEmit();
        }
    }
}

//------------------------------------------------------------------------------

void SCamera::onChooseStream()
{
    std::vector< ::arData::Camera::sptr > cameras = this->getCameras();

    for(auto& camera : cameras)
    {
        ::fwGui::dialog::InputDialog inputDialog;
        std::string streamSource;

        inputDialog.setTitle("Enter stream source");

        streamSource = inputDialog.getInput();
        if(!streamSource.empty())
        {
            camera->setCameraSource(::arData::Camera::STREAM);
            camera->setStreamUrl(streamSource);

            ::arData::Camera::ModifiedSignalType::sptr sig;
            sig = camera->signal< ::arData::Camera::ModifiedSignalType >( ::arData::Camera::s_MODIFIED_SIG );
            sig->asyncEmit();
        }
    }
}

//------------------------------------------------------------------------------

void SCamera::onChooseDevice()
{
    std::vector< ::arData::Camera::sptr > cameras = this->getCameras();

    for(auto& camera : cameras)
    {
        ::videoQt::editor::CameraDeviceDlg camDialog;
        camDialog.exec();

        bool isSelected = camDialog.getSelectedCamera(camera);
        if(isSelected)
        {
            ::arData::Camera::ModifiedSignalType::sptr sig;
            sig = camera->signal< ::arData::Camera::ModifiedSignalType >( ::arData::Camera::s_MODIFIED_SIG );
            sig->asyncEmit();
        }
    }
}

//------------------------------------------------------------------------------

std::vector< ::arData::Camera::sptr > SCamera::getCameras()
{
    std::vector< ::arData::Camera::sptr > cameras;
    if (this->isVersion2())
    {
        auto cameraSeries = this->getInOut< ::arData::CameraSeries >("cameraSeries");
        if(cameraSeries)
        {
            const size_t numCameras = cameraSeries->getNumberOfCameras();
            for(size_t i = 0; i < numCameras; ++i)
            {
                cameras.push_back(cameraSeries->getCamera(i));
            }
        }
        else
        {
            cameras.push_back(this->getInOut< ::arData::Camera >("camera"));
        }
    }
    else
    {
        // TODO: When removing this appXml1 branch we can make the method const
        cameras.push_back(this->getObject< ::arData::Camera >());
    }

    return cameras;
}

//------------------------------------------------------------------------------

} //namespace editor
} //namespace videoQt

