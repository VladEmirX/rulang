# CMake generated file
# The compiler generated pdb file needs to be written to disk
# by mspdbsrv. The foreach retry loop is needed to make sure
# the pdb file is ready to be copied.

foreach(retry RANGE 1 30)
  if (EXISTS "D:/spaceless/projs/rulang/cmake-build-debug-mingw-clang/rulang.dir/${PDB_PREFIX}rulang.pdb" AND (NOT EXISTS "D:/spaceless/projs/rulang/cmake-build-debug-mingw-clang/test_lexer_rulang.dir/${PDB_PREFIX}rulang.pdb" OR NOT "D:/spaceless/projs/rulang/cmake-build-debug-mingw-clang/test_lexer_rulang.dir/${PDB_PREFIX}rulang.pdb  " IS_NEWER_THAN "D:/spaceless/projs/rulang/cmake-build-debug-mingw-clang/rulang.dir/${PDB_PREFIX}rulang.pdb"))
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy "D:/spaceless/projs/rulang/cmake-build-debug-mingw-clang/rulang.dir/${PDB_PREFIX}rulang.pdb" "D:/spaceless/projs/rulang/cmake-build-debug-mingw-clang/test_lexer_rulang.dir/${PDB_PREFIX}" RESULT_VARIABLE result  ERROR_QUIET)
    if (NOT result EQUAL 0)
      execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1)
    else()
      break()
    endif()
  elseif(NOT EXISTS "D:/spaceless/projs/rulang/cmake-build-debug-mingw-clang/rulang.dir/${PDB_PREFIX}rulang.pdb")
    execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1)
  endif()
endforeach()
