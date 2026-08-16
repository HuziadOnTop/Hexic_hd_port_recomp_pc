// fp_fix.cpp -- mask all FP exceptions globally via VEH.
// Xbox 360 PowerPC code relies on FP exceptions being masked; x86 may raise them.

#include <windows.h>
#include <cfloat>

namespace {

LONG CALLBACK FpExceptionFilter(PEXCEPTION_POINTERS info) {
  DWORD code = info->ExceptionRecord->ExceptionCode;
  if (code >= STATUS_FLOAT_DENORMAL_OPERAND &&
      code <= STATUS_FLOAT_UNDERFLOW) {
    _controlfp(_MCW_EM, _MCW_EM);
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

struct FpFixInit {
  FpFixInit() {
    _controlfp(_MCW_EM, _MCW_EM);
    AddVectoredExceptionHandler(1, FpExceptionFilter);
  }
} g_fp_fix;

}  // namespace
