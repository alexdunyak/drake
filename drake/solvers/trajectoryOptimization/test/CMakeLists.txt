add_executable(trajectory_optimization_test trajectory_optimization_test.cc)
target_link_libraries(trajectory_optimization_test drakeTrajectoryOptimization
  ${GTEST_BOTH_LIBRARIES})
# TODO(Lucy-tri) There is currently no non-linear solver on Windows: see 
# issue #2352.
if(ipopt_FOUND OR nlopt_FOUND OR snopt_c_FOUND)
  add_test(NAME trajectory_optimization_test COMMAND 
    trajectory_optimization_test)
endif()

add_matlab_test(NAME solvers/trajectoryOptimization/test/contactImplicitBrick
  COMMAND contactImplicitBrick PROPERTIES TIMEOUT 750)
