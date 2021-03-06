
#include <cmath>

#include <memory>

#include <gtest/gtest.h>

#include "drake/examples/Pendulum/Pendulum.h"
#include "drake/systems/plants/constraint/dynamic_constraint.h"
#include "drake/util/eigen_matrix_compare.h"

using drake::util::MatrixCompareType;

GTEST_TEST(PendulumDynamicConstraint, PendulumDynamicConstraintTest) {
  auto p = make_shared<Pendulum>();

  Eigen::VectorXd x(1 + Drake::getNumStates(*p) * 2 +
                        Drake::getNumInputs(*p) * 2);
  x(0) = 0.2;          // h
  x(1) = 0;            // x0(0)
  x(2) = 0;            // x0(1)
  x(3) = 0.05 * M_PI;  // x1(0)
  x(4) = 0;            // x1(1)
  x(5) = 0.00537668;   // u0
  x(6) = 0.018339;     // u1

  drake::systems::SystemDynamicConstraint<Pendulum> dut(p);

  Drake::TaylorVecXd result;
  dut.eval(Drake::initializeAutoDiff(x), result);

  // Expected values came from running the MATLAB code for
  // PendulumPlant through the constraint function in
  // DircolTrajectoryOptimization.m and printing the results.
  EXPECT_NEAR(result(0).value(), 1.1027, 1e-4);
  EXPECT_NEAR(result(1).value(), 2.2657, 1e-4);

  Eigen::VectorXd d_0_expected(x.size());
  d_0_expected << -6.26766, -7.0095, -0.74, 7.015539, -0.76, -0.1, 0.1;
  Eigen::VectorXd d_1_expected(x.size());
  d_1_expected << 0.1508698, 14.488559, -6.715012, 14.818155,
      7.315012, -2.96, -3.04;
  EXPECT_TRUE(CompareMatrices(result(0).derivatives(), d_0_expected, 1e-4,
                              MatrixCompareType::absolute));
  EXPECT_TRUE(CompareMatrices(result(1).derivatives(), d_1_expected, 1e-4,
                              MatrixCompareType::absolute));
}
