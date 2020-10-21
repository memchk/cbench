# CBench Verilator Framework
This is a header-only library for C++ to assist creating testbenches for the Verilator simulation framework. It assists in creating reliable multi-clock simulations while wrapping some of Verilator's more annoying parts.

This is alpha quality software, I attempt to keep it bug free however expect API changes and redesigns as
they are not final. Pull requests, issues, and suggestions are welcome.

## TODO
* Create better documentation
* Implement more generic and performant scheduling algorithim.
* Simplify and rebuild API to be more consistant.
* Package for Bazel, CMake, and Conan.

## Thanks
This code takes heavy inspiration from Dr. Dan Gisselquist and his wonderful [blog](https://zipcpu.com), specifically his posts on [multi-clock](https://zipcpu.com/blog/2018/09/06/tbclock.html) simulation. I suggest anyone looking at this take a look at his posts on FPGA and digital design, they are a fantastic resource for anyone in the field.

## License
Licensed under any of

* Apache License, Version 2.0, (LICENSE.APACHE or http://www.apache.org/licenses/LICENSE-2.0)
* MIT license (LICENSE.MIT or http://opensource.org/licenses/MIT)
* CERN Open Hardware Licence v2.0 - Permissive (LICENSE.OHL-P or https://ohwr.org/cern_ohl_p_v2.txt)

at your option.