#pragma once

#include "nfsmw_perfil_cpu.h"

#include <cstdint>
#include <utility>

namespace nfsmw::hilos {
int64_t CpuHiloUs(uint64_t manejador);
}

namespace nfsmw::perfil_subetapas {

constexpr unsigned kEtapas = 6;
constexpr unsigned kEtapasVirtual = 15;

#if defined(__SWITCH2__)
inline thread_local bool activa = false;
inline thread_local uint64_t manejador = 0;
inline thread_local uint64_t visitas = 0;
inline thread_local perfil_cpu::Etapa etapas[kEtapas];
inline thread_local bool virtual_activa = false;
inline thread_local uint64_t visitas_virtual = 0;
inline thread_local perfil_cpu::Etapa etapas_virtual[kEtapasVirtual];
#endif

inline void Marcar() {
#if defined(__SWITCH2__)
  if (activa) [[unlikely]] {
    ++visitas;
  }
#endif
}

inline void MarcarVirtual() {
#if defined(__SWITCH2__)
  if (virtual_activa) [[unlikely]] {
    ++visitas_virtual;
  }
#endif
}

template <unsigned etapa, typename F>
inline void Medir(F&& llamada) {
  static_assert(etapa < kEtapas);
#if defined(__SWITCH2__)
  if (activa) [[unlikely]] {
    const int64_t referencia = hilos::CpuHiloUs(manejador);
    const int64_t inicio = hilos::CpuHiloUs(manejador);
    std::forward<F>(llamada)();
    const int64_t fin = hilos::CpuHiloUs(manejador);
    etapas[etapa].Agregar(referencia, inicio, fin);
    return;
  }
#endif
  std::forward<F>(llamada)();
}

template <unsigned etapa, typename F>
inline void MedirVirtual(F&& llamada) {
  static_assert(etapa < kEtapasVirtual);
#if defined(__SWITCH2__)
  if (virtual_activa) [[unlikely]] {
    const int64_t referencia = hilos::CpuHiloUs(manejador);
    const int64_t inicio = hilos::CpuHiloUs(manejador);
    std::forward<F>(llamada)();
    const int64_t fin = hilos::CpuHiloUs(manejador);
    etapas_virtual[etapa].Agregar(referencia, inicio, fin);
    return;
  }
#endif
  std::forward<F>(llamada)();
}

}  // namespace nfsmw::perfil_subetapas
