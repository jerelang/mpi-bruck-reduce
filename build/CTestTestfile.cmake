# CMake generated Testfile for 
# Source directory: /Users/jere/Documents/uni/dissemination_reduce/sources
# Build directory: /Users/jere/Documents/uni/dissemination_reduce/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(alg_baseline "/opt/homebrew/bin/mpiexec" "-n" "4" "/Users/jere/Documents/uni/dissemination_reduce/build/dissemination_reduce" "128" "0" "--check" "--warmup" "0" "--repeat" "1")
set_tests_properties(alg_baseline PROPERTIES  ENVIRONMENT "OMPI_MCA_rmaps_base_oversubscribe=1" _BACKTRACE_TRIPLES "/Users/jere/Documents/uni/dissemination_reduce/sources/CMakeLists.txt;31;add_test;/Users/jere/Documents/uni/dissemination_reduce/sources/CMakeLists.txt;0;")
add_test(alg_bruck "/opt/homebrew/bin/mpiexec" "-n" "4" "/Users/jere/Documents/uni/dissemination_reduce/build/dissemination_reduce" "128" "1" "--check" "--warmup" "0" "--repeat" "1")
set_tests_properties(alg_bruck PROPERTIES  ENVIRONMENT "OMPI_MCA_rmaps_base_oversubscribe=1" _BACKTRACE_TRIPLES "/Users/jere/Documents/uni/dissemination_reduce/sources/CMakeLists.txt;33;add_test;/Users/jere/Documents/uni/dissemination_reduce/sources/CMakeLists.txt;0;")
add_test(alg_circulant "/opt/homebrew/bin/mpiexec" "-n" "4" "/Users/jere/Documents/uni/dissemination_reduce/build/dissemination_reduce" "128" "2" "--check" "--warmup" "0" "--repeat" "1")
set_tests_properties(alg_circulant PROPERTIES  ENVIRONMENT "OMPI_MCA_rmaps_base_oversubscribe=1" _BACKTRACE_TRIPLES "/Users/jere/Documents/uni/dissemination_reduce/sources/CMakeLists.txt;35;add_test;/Users/jere/Documents/uni/dissemination_reduce/sources/CMakeLists.txt;0;")
