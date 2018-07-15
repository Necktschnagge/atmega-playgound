# Namespaces

* fsl
   * con (container classes especially for microcontroller)
      * istack
      * stack
   * dev (communication for _devices_, such as multi sensor measurement tool, pump controller)
   * hw (hardware, hardware abstraction)
      * gpio_pin
   * lg (logical stuff, that has no connection to the microcontroller at all)
      * flags<...>
      * f_order.h (for min, max)
      * range_int<...>
      * single_flags
   * os (typical operating system features: like scheduler, taskrunner)
      * scheduler
      * system_time
   * str (structural stuff, classes etc)
      * callable
      * const_callable
      * exceptional<...>
      * void_function
   * ui (user interface stuff)
      * matrix_keyboard
   * util (utilities)
      * byte_swap
