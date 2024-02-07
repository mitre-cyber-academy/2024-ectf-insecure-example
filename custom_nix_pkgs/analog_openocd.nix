/**
 * @file "analog_openocd.nix"
 * @author Frederich Stine 
 * @brief Nix build for Analog Devices fork of OpenOCD
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

{ stdenv
, lib
, pkg-config
, hidapi
, jimtcl
, libjaylink
, libusb1
, libgpiod
, gcc
, gnumake
, coreutils
, autoconf
, automake
, texinfo
, git
, libtool
, which
, libftdi1
}:

stdenv.mkDerivation {
  pname = "openocd-analog";
  version = "0.12.0";

  src = builtins.fetchGit {
    url = "https://github.com/analogdevicesinc/openocd.git";
    ref = "release";
    submodules = true;
  };

  nativeBuiltInputs = [ pkg-config ];

  buildInputs = [
    hidapi
    gcc
    gnumake
    coreutils
    pkg-config
    autoconf
    automake
    texinfo
    git
    jimtcl
    libusb1
    libjaylink
    libftdi1
    libtool
    which
  ];

  postPatch = ''
    substituteInPlace src/jtag/drivers/libjaylink/autogen.sh --replace "LIBTOOLIZE=glibtoolize" "LIBTOOLIZE=libtoolize"
  '';

  enableParallelBuilding = true;

  hardeningDisable = [ "fortify" ];

  configurePhase = ''
    SKIP_SUBMODULE=1 ./bootstrap
    ./configure --prefix=$out --disable-werror
  '';
   
   meta = with lib; {
    description = "OpenOCD fork for Analog Devices microcontrollers";
    longDescription = ''
      This is a fork of OpenOCD by ADI,
      which brings support to MAXIM MCUs microcontroller.
    '';
    homepage = "https://github.com/analogdevicesinc/openocd.git";
    license = licenses.gpl2Plus;
    maintainers = with maintainers; [ eCTF ];
  };
}
