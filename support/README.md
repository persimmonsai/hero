# Support
Contains the support libraries and driver to interface the PULP accelerator from the host. More in particular contains the following parts:
* `pulp-driver`: The driver to interface PULP from the host. Manages the core control and state that needs to be handled from a kernel level perspective and should be managed to ensure the accelerator remains functional and the host cannot hang.
* `libpulp`: The user-space for PULP to bridge the kernel driver with applications, used either via the standalone app to offload standalone applications or via the heterogeneous toolchain for automatic offloading.
* `libhero-target`: heterogeneous library with the same functions implemented for both the host and the accelerator. Can be used in heterogeneous offload sections that traditionally cannot use accelerator libraries directly as the offload could also be run on the host. In GCC it is needed to do this with different libraries during linking, but with the new LLVM setup it would also be possible to reimplement it with header-only libraries.
* `snitch-driver`: The driver to interface Snitch from the host. Snitch-equivalent of `pulp-driver`
* `libsnitch`: The user-space library for Snitch to bridge the kernel driver with applications, Snitch-equivalent of `libpulp`
