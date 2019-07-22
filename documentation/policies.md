# Do's and Don't's when working in this repository

## C++ programming policies

* volatile functions should never return a reference to volatile *this if the return value may stay unused.


## git usage

* naming of branches

  | feature-* | _new features_ |
  | ---------:|:-------------- |
  | fix-* | _fixes for features already merged into master_ |
  | docu-* | _documentary stuff_ |
  | structure-* | _structural stuff for the repository_ |
  | experimental-* | _experimental releases_ |
