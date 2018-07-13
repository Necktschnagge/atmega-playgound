# Namespaces

* fsl
   * ui (user interface stuff)
      * matrix_keyboard
   * hw (hardware, hardware abstraction)
      * gpio_pin
   * os (typical operating system features: like systime, scheduler, taskrunner)
      * ...
   * lg (logical stuff, that has no connection to the microcontroller at all)
      * range_int<...>
      * single_flags
      * flags<...>
   * str (structural stuff, classes etc)
      * void_function
      * callable
      * const_callable
      * exceptional<...>
   * con (container classes especially for microcontroller)
      * stack
      * istack
   * dev (communication for _devices_, such as multi sensor measurement tool, pump controller)

fsl::boost (?)
