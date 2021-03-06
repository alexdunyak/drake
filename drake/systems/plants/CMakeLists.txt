# TODO(#2852) We can't yet handle shadowing as a compiler error.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS_NO_ERROR_SHADOW}")

# IMPORTANT NOTE!!
# matlab has it's own boost libraries.  DO NOT let any mex file depend
# on the system boost (directly nor indirectly), or you're asking for trouble.

add_library_with_exports(LIB_NAME drakeXMLUtil SOURCE_FILES xmlUtil.cpp)
target_link_libraries(drakeXMLUtil drakeCommon tinyxml2 spruce)
pods_install_libraries(drakeXMLUtil)
drake_install_headers(xmlUtil.h)
pods_install_pkg_config_file(drake-xml-util
  LIBS -ldrakeXMLUtil -ldrakeCommmon -ltinyxml2 -lspruce
  REQUIRES
  VERSION 0.0.1)

add_library_with_exports(LIB_NAME drakeRBM SOURCE_FILES
  parser_sdf.cc
  parser_urdf.cc
  RigidBodyTree.cpp
  RigidBody.cpp
  RigidBodyTreeContact.cpp
  rigid_body_frame.cc)
target_link_libraries(drakeRBM drakeCollision drakeJoints drakeUtil drakeXMLUtil)
pods_install_libraries(drakeRBM)
drake_install_headers(
  ConstraintWrappers.h
  ForceTorqueMeasurement.h
  KinematicPath.h
  KinematicsCache.h
  material_map.h
  pose_map.h
  RigidBodyFrame.h
  RigidBody.h
  RigidBodyTree.h
  parser_sdf.h
  parser_urdf.h)

pods_install_pkg_config_file(drake-rbm
  LIBS -ldrakeRBM -ldrakeCollision -ldrakeJoints -ldrakeUtil -ldrakeXMLUtil
  REQUIRES
  VERSION 0.0.1)

pods_find_pkg_config(gurobi)
pods_find_pkg_config(snopt_c)
add_subdirectory(shapes)
add_subdirectory(collision)
add_subdirectory(constraint)
add_subdirectory(joints)

if(MATLAB_FOUND)
  add_mex(rbmMexFunctions SHARED rigidBodyTreeMexFunctions.cpp)
  target_link_libraries(rbmMexFunctions drakeRBM drakeMexUtil)
  # These warnings are not worth fixing.
  set_target_properties(
    rbmMexFunctions PROPERTIES COMPILE_FLAGS ${CXX_FLAGS_NO_SIGN_COMPARE})

  macro(add_rbm_mex)
    # TODO: use this again once I can assume everyone has CMAKE version >= 2.8.8
    # add_mex(${ARGV} ${ARGV}.cpp $<TARGET_OBJECTS:drakeUtil>)
    # target_link_libraries(${ARGV} drakeRBM)
    add_mex(${ARGV} ${ARGV}.cpp)
    target_link_libraries(${ARGV} rbmMexFunctions)
    # These warnings are not worth fixing.
    set_target_properties(
      ${ARGV} PROPERTIES COMPILE_FLAGS ${CXX_FLAGS_NO_SIGN_COMPARE})
  endmacro()

  add_rbm_mex(constructModelmex)
  add_rbm_mex(collisionDetectmex)
  add_rbm_mex(smoothDistancePenaltymex)
  add_rbm_mex(collisionRaycastmex)
  add_rbm_mex(allCollisionsmex)
  add_rbm_mex(collidingPointsmex)
  add_rbm_mex(collidingPointsCheckOnlymex)
  add_rbm_mex(findKinematicPathmex)
  add_rbm_mex(geometricJacobianmex)
  add_rbm_mex(doKinematicsmex)
  add_rbm_mex(forwardKinmex)
  add_rbm_mex(forwardKinJacobianmex)
  add_rbm_mex(forwardKinPositionGradientmex)
  add_rbm_mex(centerOfMassmex)
  add_rbm_mex(centerOfMassJacobianmex)
  add_rbm_mex(centroidalMomentumMatrixmex)
  add_rbm_mex(massMatrixmex)
  add_rbm_mex(dynamicsBiasTermmex)
  add_rbm_mex(geometricJacobianDotTimesVmex)
  add_rbm_mex(centroidalMomentumMatrixDotTimesVmex)
  add_rbm_mex(centerOfMassJacobianDotTimesVmex)
  add_rbm_mex(forwardJacDotTimesVmex)
  add_rbm_mex(contactConstraintsmex)
  add_rbm_mex(surfaceTangentsmex)
  add_rbm_mex(jointLimitConstraintsmex)
  add_rbm_mex(positionConstraintsmex)
  add_rbm_mex(testFrameParsermex)
  add_rbm_mex(computeS1Trajmex)
  add_rbm_mex(createKinematicsCachemex)
  add_rbm_mex(createKinematicsCacheAutoDiffmex)
  add_rbm_mex(velocityToPositionDotMappingmex)
  add_rbm_mex(positionDotToVelocityMappingmex)
  add_rbm_mex(collisionDetectFromPointsmex)
  target_link_libraries(computeS1Trajmex drakeZMPUtil)

  if(gurobi_FOUND)
    add_definitions(-DPATHLCP_MEXFILE=\"${PROJECT_SOURCE_DIR}/thirdParty/misc/pathlcp/lcppath.${MEX_EXT}\")
    add_mex(solveLCPmex solveLCPmex.cpp)
    target_link_libraries(solveLCPmex drakeRBM drakeMexUtil drakeQP)
  endif()
endif()

add_library_with_exports(LIB_NAME drakeIKoptions SOURCE_FILES IKoptions.cpp)
target_link_libraries(drakeIKoptions drakeRBM)

if(MATLAB_FOUND)
  macro(add_ikoptions_mex)
    add_mex(${ARGV} ${ARGV}.cpp)
    target_link_libraries(${ARGV} drakeRBM drakeMexUtil drakeIKoptions)
    # These warnings are not worth fixing.
    set_target_properties(
      ${ARGV} PROPERTIES COMPILE_FLAGS ${CXX_FLAGS_NO_SIGN_COMPARE})
  endmacro()
  add_ikoptions_mex(IKoptionsmex)

  macro(add_constraint_mex)
    add_mex(${ARGV} ${ARGV}.cpp)
    target_link_libraries(${ARGV} drakeRBM drakeUtil drakeRigidBodyConstraint)
  endmacro()
endif()

set(drakeIK_SRC_FILES inverseKin.cpp inverseKinBackend.cpp
  inverseKinPointwise.cpp inverseKinTraj.cpp)
set(drakeIK_PODS_PKG)
if(gurobi_FOUND)
  set(drakeIK_SRC_FILES ${drakeIK_SRC_FILES} approximateIK.cpp)
  set(drakeIK_PODS_PKG ${drakeIK_PODS_PKG} gurobi)
endif()
if(snopt_c_FOUND)
  set(drakeIK_SRC_FILES ${drakeIK_SRC_FILES} inverseKinSnoptBackend.cpp)
  set(drakeIK_PODS_PKG ${drakeIK_PODS_PKG} snopt_c)
else()
  set(drakeIK_SRC_FILES ${drakeIK_SRC_FILES} inverseKinNoSnoptBackend.cpp)
endif()

if(drakeIK_SRC_FILES)
  add_library_with_exports(LIB_NAME drakeIK SOURCE_FILES ${drakeIK_SRC_FILES})
  pods_use_pkg_config_packages(drakeIK ${drakeIK_PODS_PKG})
  target_link_libraries(drakeIK drakeRBM drakeOptimization drakeRigidBodyConstraint drakeIKoptions)
  pods_install_libraries(drakeIK)
  pods_install_libraries(drakeIKoptions)
  drake_install_headers(RigidBodyIK.h IKoptions.h inverseKinBackend.h)
  pods_install_pkg_config_file(drake-ik
    LIBS -ldrakeIK -ldrakeRigidBodyConstraint -ldrakeIKoptions
    REQUIRES drake-rbm
    VERSION 0.0.1)
endif()

if(MATLAB_FOUND)
  if(gurobi_FOUND)
    add_mex(approximateIKmex approximateIKmex.cpp)
    target_link_libraries(approximateIKmex drakeIK drakeRBM drakeMexUtil drakeRigidBodyConstraint)
    pods_use_pkg_config_packages(approximateIKmex gurobi)
  endif()

  macro(add_ik_mex)
    add_mex(${ARGV} ${ARGV}.cpp)
    target_link_libraries(${ARGV} drakeIK drakeRBM drakeMexUtil drakeRigidBodyConstraint)
    # These warnings are not worth fixing.
    set_target_properties(
      ${ARGV} PROPERTIES COMPILE_FLAGS ${CXX_FLAGS_NO_SIGN_COMPARE})
  endmacro()

  if(snopt_c_FOUND)
    add_ik_mex(inverseKinmex)
    add_ik_mex(inverseKinPointwisemex)
    add_ik_mex(inverseKinTrajmex)
  endif()
endif()

add_library_with_exports(LIB_NAME drakeRBSystem SOURCE_FILES RigidBodySystem.cpp)
target_link_libraries(drakeRBSystem drakeRBM drakeOptimization drakeRigidBodyConstraint)

drake_install_headers(RigidBodySystem.h)
pods_install_libraries(drakeRBSystem)
pods_install_pkg_config_file(drake-rbsystem
  CFLAGS -I${CMAKE_INSTALL_PREFIX}/include
  LIBS -ldrakeRBSystem -ldrakeRBM -ldrakeOptimization -ldrakeRigidBodyConstraint
  VERSION 0.0.1)

if(lcm_FOUND)
  add_executable(rigidBodyLCMNode rigidBodyLCMNode.cpp)
  target_link_libraries(rigidBodyLCMNode drakeRBSystem drakeLCMSystem threads)
  pods_install_executables(rigidBodyLCMNode)
  drake_install_headers(BotVisualizer.h)
endif()

add_subdirectory(test)
