




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       July 5, 2006
//
//  Copyrights: Imaging Technologies Inc.
//
//  Product:    Intelli-Ribbon Control
//  
//  Subsystem:  Camera System
//  -------------------------------------------
//
//
//      CONFIDENTIAL DOCUMENT
//
//      Property of Imaging Technologies Inc.
//
//


////////////////////////////////////////////////////////////////////

#include <vcrt.h>
#include <vclib.h>
#include <macros.h>
#include <sysvar.h>


////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////


#include "cameraconfig.h"



////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

SystemConfig    requestedSystemConfiguration,
currentSystemConfiguration;

//

SystemConfig    * GetCurrSystemConfig(void)
{
    return &currentSystemConfiguration;
}

void            SetCurrSystemConfig(SystemConfig *sysCfg)
{
    currentSystemConfiguration = (*sysCfg);
}

//

SystemConfig    * GetReqSystemConfig(void)
{
    return &requestedSystemConfiguration;
}

void            SetReqSystemConfig(SystemConfig *sysCfg)
{
    requestedSystemConfiguration = (*sysCfg);
}



////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

// Camera Address

void    SetTcpIpAddress(CameraAddress *camAddr, DWORD tcpIpAddr)
{
    camAddr->tcpIpAddress = tcpIpAddr;
}

DWORD   GetTcpIpAddress(CameraAddress *camAddr)
{
    return camAddr->tcpIpAddress;
}

void    SetDescription(CameraAddress *camAddr, char *setDesc)
{
    strcpy(&camAddr->description, setDesc,
           CAMERA_DESCRIPTION_LENGTH);
}

void    GetDescription(CameraAddress *camAddr, char *returnDesc)
{
    strcpy(returnDesc, &camAddr->description,
           CAMERA_DESCRIPTION_LENGTH);
}


// Versions

void    SetFpga(CameraVersion *camVer, DWORD fpgaVer)
{
    camVer->fpga = fpgaVer;
}

DWORD   GetFpga(CameraVersion *camVer)
{
    return camVer->fpga;
}

void    SetFirmware(CameraVersion *camVer, DWORD firmwareVer)
{
    camVer->firmware = firmwareVer;
}

DWORD   GetFirmware(CameraVersion *camVer)
{
    return camVer->firmware;
}

void    SetOperatingSystem(CameraVersion *camVer, DWORD osVer)
{
    camVer->operatingSystem = osVer;
}

DWORD   GetOperatingSystem(CameraVersion *camVer)
{
    return camVer->operatingSystem;
}


// Image Capture

void    SetPixelSize(ImageCapture *iCaptCfg, DWORD pixelS)
{
    iCaptCfg->pixelSize = pixelS;
}

DWORD   GetPixelSize(ImageCapture *iCaptCfg)
{
    return iCaptCfg->pixelSize;
}

void    SetShutterOpenTime(ImageCapture *iCaptCfg, DWORD shutterOT)
{
    iCaptCfg->shutterOpenTime = shutterOT;
}

DWORD   GetShutterOpenTime(ImageCapture *iCaptCfg)
{
    return iCaptCfg->shutterOpenTime;
}


// Encoder 

void    SetCutOffLength(Encoder *encCfg, DWORD cutOffLen)
{
    encCfg->cutOffLength = cutOffLen;
}

DWORD   GetCutOffLength(Encoder *encCfg)
{
    return encCfg->cutOffLength;
}

void    SetApulsesPerImpression(Encoder *encCfg, DWORD aPulsesPerImp)
{
    encCfg->aPulsesPerImpression = aPulsesPerImp;
}

DWORD   GetApulsesPerImpression(Encoder *encCfg)
{
    return encCfg->aPulsesPerImpression;
}

void    SetTwiceAroundOption(Encoder *encCfg, BOOL twiceAroundOpt)
{
    encCfg->twiceAroundOption =twiceAroundOpt;
}

BOOL    GetTwiceAroundOption(Encoder *encCfg)
{
    return encCfg->twiceAroundOption;
}

void    SetFilterSelect(Encoder *encCfg, DWORD filtSel)
{
    //   encCfg->filterSelect = filtSel;
}

DWORD   GetFilterSelect(Encoder *encCfg)
{
//    return encCfg->filterSelect;
}

void    SetShutterTriggerDelayTime(ImageCapture *imgCfg, DWORD shutterTDT)
{
    imgCfg->shutterTriggerDelayTime = shutterTDT;
}

DWORD   GetShutterTriggerDelayTime(ImageCapture *imgCfg)
{
    return imgCfg->shutterTriggerDelayTime;
}



// Transports

void    SetEnable(Transport *tranCfg, BOOL en)
{
    tranCfg->enabled = en;
}

BOOL    GetEnable(Transport *tranCfg)
{
    return tranCfg->enabled;
}

void    
SetStepMotorDirection(Transport *tranCfg, STEP_MOTOR_DIRECTION stepDir)
{
    tranCfg->stepMotorDirection = stepDir;
}

STEP_MOTOR_DIRECTION        
GetStepMotorDirection(Transport *tranCfg)
{
    return tranCfg->stepMotorDirection;
}

void    SetHomeLocation(Transport *tranCfg, DWORD homeLoc)
{
    tranCfg->homeDirection = homeLoc;
}

DWORD    GetHomeLocation(Transport *tranCfg)
{
    return tranCfg->homeDirection;
}

void    SetDistanceFromHome(Transport *tranCfg, DWORD distFromHome)
{
    tranCfg->distanceFromHome = distFromHome;
}

DWORD   GetDistanceFromHome(Transport *tranCfg)
{
    return tranCfg->distanceFromHome;
}

void    SetTransportLength(Transport *tranCfg, DWORD transportLen)
{
    tranCfg->transportLength = transportLen;
}

DWORD   GetTransportLength(Transport *tranCfg)
{
    return tranCfg->transportLength;
}

void    SetMicronsPerStep(Transport *tranCfg, DWORD micronsPerSt)
{
    tranCfg->micronsPerStep = micronsPerSt;
}

DWORD   GetMicronsPerStep(Transport *tranCfg)
{
    return tranCfg->micronsPerStep;
}

void    SetStartSpeed(Transport *tranCfg, DWORD startSp)
{
    tranCfg->startSpeed = startSp;
}

DWORD   GetStartSpeed(Transport *tranCfg)
{
    return tranCfg->startSpeed;
}

void    SetMaxSpeed(Transport *tranCfg, DWORD maxSp)
{
    tranCfg->maxSpeed = maxSp;
}

DWORD   GetMaxSpeed(Transport *tranCfg)
{
    return tranCfg->maxSpeed;
}

void    SetMaxAcceleration(Transport *tranCfg, DWORD maxAccel)
{
    tranCfg->maxAcceleration = maxAccel;
}

DWORD   GetMaxAcceleration(Transport *tranCfg)
{
    return tranCfg->maxAcceleration;
}



//
// MarkSearch Config
//

void    SetLimCircumSearchRange(MarkSearch *markSearchCfg, DWORD limCircSearchRange)
{
    markSearchCfg->limCircumSearchRange =
    limCircSearchRange;
}

DWORD   GetLimCircumSearchRange(MarkSearch *markSearchCfg)
{
    return
    markSearchCfg->limCircumSearchRange;
}

void    SetLimCircumSearchStepSize(MarkSearch *markSearchCfg, DWORD limCircSearchStepSize)
{
    markSearchCfg->limCircumSearchStepSize =
    limCircSearchStepSize;
}

DWORD   GetLimCircumSearchStepSize(MarkSearch *markSearchCfg)
{
    return 
    markSearchCfg->limCircumSearchStepSize;    
}

void    SetLimLateralSearchRange(MarkSearch *markSearchCfg, DWORD limLatSearchRange)
{
    markSearchCfg->limLateralSearchRange =
    limLatSearchRange;
}

DWORD   GetLimLateralSearchRange(MarkSearch *markSearchCfg)
{
    return
    markSearchCfg->limLateralSearchRange;    
}

void    SetLimLateralSearchStepSize(MarkSearch *markSearchCfg, DWORD limLatSearchStepSize)
{
    markSearchCfg->limLateralSearchStepSize =
    limLatSearchStepSize;
}

DWORD   GetLimLateralSearchStepSize(MarkSearch *markSearchCfg)
{
    return
    markSearchCfg->limLateralSearchStepSize;
}

//
// Camera Orientation Config
//

void                    
SetCircDirection(CameraDirection *CameraDirCfg, CAMERA_CIRCUM_DIRECTION circDir)
{
    CameraDirCfg->circDirection = circDir; 
}

CAMERA_CIRCUM_DIRECTION 
GetWebDirection(CameraDirection *CameraDirCfg)
{
    return CameraDirCfg->circDirection;
}

void                        
SetLateralOrientation(CameraDirection *CameraDirCfg, CAMERA_LATERAL_DIRECTION latDir)
{
    CameraDirCfg->latDirection = latDir;
}

CAMERA_LATERAL_DIRECTION    
GetLateralOrientation(CameraDirection *CameraDirCfg)
{
    return CameraDirCfg->latDirection;
}

//
//
// System Camera Config
//
//

void                
SetCameraAddress(SystemConfig *sysCfg, CameraAddress camAddr)
{
    sysCfg->address = camAddr;
}

CameraAddress       
GetCameraAddress(SystemConfig *sysCfg)
{
    return sysCfg->address;
}

void                
SetCameraVersion(SystemConfig *sysCfg, CameraVersion camVer)
{
    sysCfg->version = camVer;
}

CameraVersion       
GetCameraVersion(SystemConfig *sysCfg)
{
    return sysCfg->version;
}


void                
SetImageCapture(SystemConfig *sysCfg, ImageCapture iCapt)
{
    sysCfg->imageCapture = iCapt;
}

ImageCapture  GetImageCapture(SystemConfig *sysCfg)
{
    return sysCfg->imageCapture;
}

void
SetEncoder(SystemConfig *sysCfg, Encoder eCfg)
{
    sysCfg->encoder = eCfg;
}

Encoder       GetEncoder(SystemConfig *sysCfg)
{
    return sysCfg->encoder;
}

void                
SetTransport(SystemConfig *sysCfg, Transport tCfg)
{
    sysCfg->transport = tCfg;
}

Transport     GetTransport(SystemConfig *sysCfg)
{
    return sysCfg->transport;
}

void                
SetLimitedMarkSearch(SystemConfig *sysCfg, MarkSearch markSearchCfg)
{
    sysCfg->markSearch = markSearchCfg;
}

MarkSearch        
GetLimitedMarkSearch(SystemConfig *sysCfg)
{

    return sysCfg->markSearch;
}



void                    
SetCameraDirection
(SystemConfig *sysCfg, CameraDirection camDirCfg)
{
    sysCfg->cameraDirection = camDirCfg;
}

CameraDirection 
GetCameraDirection(SystemConfig *sysCfg)
{
    return sysCfg->cameraDirection;
}


////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////

void                    
SetCameraImaging(SystemConfig *sysCfg,  CameraImaging cameraImg)
{
    sysCfg->cameraImaging = cameraImg;
}

CameraImaging   
GetCameraImaging(SystemConfig *sysCfg)
{
    return sysCfg->cameraImaging;
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

BYTE            GetReferenceMarkIndex(void)
{
    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    return camImagingPtr->refIndex;
}

void            SetReferenceMarkIndex(BYTE refIndex)
{
    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    camImagingPtr->refIndex = refIndex;
}

void    InitializeSystemConfiguration(void)
{
    Encoder *encCfgPtr;
    ImageCapture    *imgCaptureCfg;
    //


    // TODO put in encoder file
    // Encoder Default
    memset(&currentSystemConfiguration, 0, sizeof(SystemConfig));

    encCfgPtr       = &currentSystemConfiguration.encoder;
    imgCaptureCfg   =    &currentSystemConfiguration.imageCapture;

    // SetCutOffLength(encCfgPtr, 1160018); // Conley
    // SetApulsesPerImpression(encCfgPtr, 1024);
    SetCutOffLength(encCfgPtr, 638372); // 8 inch spin fixture
    SetApulsesPerImpression(encCfgPtr, 4096);
    SetTwiceAroundOption(encCfgPtr, FALSE);
    SetFilterSelect(encCfgPtr, 0);
    SetShutterTriggerDelayTime(imgCaptureCfg, 500);
}
