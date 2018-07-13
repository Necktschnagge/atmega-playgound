# Namespaces

* fsl
   * ui (user interface stuff)
      * matrix_keyboard
   * hw (hardware, hardware abstraction)
      * gpio_pin
   * os (typical operating system features: like scheduler, taskrunner)
      * scheduler
      * system_time
   * lg (logical stuff, that has no connection to the microcontroller at all)
      * flags<...>
      * range_int<...>
      * single_flags
   * str (structural stuff, classes etc)
      * callable
      * const_callable
      * exceptional<...>
      * void_function
   * con (container classes especially for microcontroller)
      * istack
      * stack
   * dev (communication for _devices_, such as multi sensor measurement tool, pump controller)
