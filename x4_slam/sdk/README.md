# SDK Layout

`CameraSDK` is a symlink to the official Insta360 ARM64 CameraSDK:

```text
/home/deep/peize/where_is_my_key/ref_repo/src/Linux_CameraSDK-2.1.1_MediaSDK-3.1.1/CameraSDK-20251105_140609-2.1.1-gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu
```

Use this SDK first on the DGX Spark. It contains AArch64 `CameraSDKTest` and `libCameraSDK.so`.

Do not use the x86_64 `CameraSDK-2.1.1-Linux` or the amd64-only MediaSDK package on this machine.
