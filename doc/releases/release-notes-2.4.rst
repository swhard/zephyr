:orphan:

.. _zephyr_2.4:

Zephyr 2.4.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr RTOS version 2.4.0.

Major enhancements with this release include:


The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:


More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.


API Changes
***********

Deprecated in this release
==========================


Removed APIs in this release
============================


Stable API changes in this release
==================================


Kernel
******


Architectures
*************

* ARC:


* ARM:


* POSIX:


* RISC-V:


* x86:


Boards & SoC Support
********************

* Added support for these SoC series:


* Added support for these ARM boards:


* Made these changes in other boards


* Added support for these following shields:


Drivers and Sensors
*******************

* ADC


* Audio


* Bluetooth


* CAN


* Clock Control


* Console


* Counter


* Crypto


* DAC


* Debug


* Display


* DMA


* EEPROM


* Entropy


* ESPI


* Ethernet


* Flash


* GPIO


* Hardware Info


* I2C


* I2S


* IEEE 802.15.4


* Interrupt Controller


* IPM


* Keyboard Scan


* LED


* LED Strip


* LoRa


* Modem


* PECI


* Pinmux


* PS/2


* PWM


* Sensor


* Serial


* SPI


* Timer


* USB


* Video


* Watchdog


* WiFi



Networking
**********


Bluetooth
*********

* Host:


* BLE split software Controller:


Build and Infrastructure
************************

* Devicetree

Libraries / Subsystems
**********************

* Disk


* Random


* POSIX subsystem:


* Power management:


HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

Documentation
*************


Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.3.0 tagged
release:
