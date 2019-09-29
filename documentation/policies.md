# Do's and Don't's when working in this repository

## C++ programming policies

* volatile functions should never return a reference to volatile *this if the return value may stay unused.


## git usage

### naming of branches

#### Names of branches should follow the following style:
`<kind>-<description>[-<description>]*[-<version/etc>]`

  | pattern | purpose |
  | -------:|:------- |
  | feature-* | _new features_ |
  | fix-* | _fixes for features already merged into master_ |
  | docu-* | _documentary stuff_ |
  | structure-* | _structural stuff for the repository_ |
  | experimental-* | _experimental releases_ |
  | release-* | _productive releases_ |

#### For example name feature branches this way:
* `feature-scheduler-v2`
* `feature-iopin`
* `release-1.0`
* `experimental-scheduler-demo`
* `fix-scheduler-compiler-errors`
* `fix-scheduler-runtime-behavior`
