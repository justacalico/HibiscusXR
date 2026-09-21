package com.picovr.picovrlib.cvcontrollerlib;

// Callbacks pushed by CVControllerService. Method order is fixed: the
// stock service resolves calls by transaction id, so the declarations
// below mirror the original interface exactly.
interface ICVAIDLServiceCallback {
    void feedbackConnectStatus(int controller, int status);
    void feedbackDeviceInfo(String info1, String info2, int flag);
    void feedbackMainControllerSerialNumChanged(int controllerSerialNum);
    void feedbackControllerThreadStarted();
    void feedbackControllerDeviceVersion(int device, String version);
    void feedbackControllerControllerSn(int device, String sn);
    void feedbackControllerUnbind(int status);
    void feedbackControllerStatus(int status);
    void feedbackControllerBusyStatus(int status);
    void feedbackControllerOTAStatusCode(int device, int code);
    void feedbackControllerDeviceVersionSN(int device, String versionSn);
    void feedbackControllerUniqueIdentifier(String identifier);
    void feedbackControllerCombinedKeyUnbind(int controllerSerialNum);
    void feedbackStationOTAProgress(int progress);
    void feedbackStationOTAErroCode(int code);
    void feedbackControllerOTAProgress(int device, int progress, int status);
    void feedbackControllerOTAErroCode(int device, int code, int status);
    void feedbackOTAComplete(int status);
    void feedbackControllerDeviceBleMac(int device, String mac);
    void feedbackControllerBlePacketLossRate(int device, float lossRate);
    void feedbackHandNessSerialNumChanged(int controllerSerialNum);
    void feedbackDeviceChannel(int device, int channel);
    void feedbackControllerNDIVersion(int device, String version);
}
