#pragma once

#include <cstdint>

namespace nfsmw::perfil_cpu {

enum class Estado { kInicio, kDisponible, kNoDisponible };

struct Muestra {
  Estado estado;
  double segundos;
  double ms_cpu_por_segundo;
};

struct Etapa {
  uint64_t muestras = 0;
  uint64_t fallos = 0;
  uint64_t cpu_us = 0;
  uint64_t referencia_us = 0;

  void Agregar(int64_t referencia, int64_t inicio, int64_t fin) {
    if (referencia < 0 || inicio < referencia || fin < inicio) {
      ++fallos;
      return;
    }
    ++muestras;
    referencia_us += uint64_t(inicio - referencia);
    cpu_us += uint64_t(fin - inicio);
  }
};

template <unsigned Capacidad>
struct Destinos {
  struct Entrada {
    uint32_t destino = 0;
    Etapa etapa;
  };
  Entrada entradas[Capacidad]{};
  unsigned usados = 0;
  uint64_t descartados = 0;

  void Agregar(uint32_t destino, int64_t referencia, int64_t inicio, int64_t fin) {
    for (unsigned i = 0; i < usados; ++i) {
      if (entradas[i].destino == destino) {
        entradas[i].etapa.Agregar(referencia, inicio, fin);
        return;
      }
    }
    if (usados == Capacidad) {
      ++descartados;
      return;
    }
    auto& entrada = entradas[usados++];
    entrada.destino = destino;
    entrada.etapa.Agregar(referencia, inicio, fin);
  }
};

class Ventana {
 public:
  Muestra Actualizar(int64_t ahora_ns, int64_t cpu_us) {
    if (cpu_us < 0 || ahora_ns < 0) {
      valida_ = false;
      return {Estado::kNoDisponible, 0.0, 0.0};
    }
    if (!valida_ || ahora_ns <= antes_ns_ || cpu_us < antes_cpu_us_) {
      antes_ns_ = ahora_ns;
      antes_cpu_us_ = cpu_us;
      valida_ = true;
      return {Estado::kInicio, 0.0, 0.0};
    }
    const double segundos = double(ahora_ns - antes_ns_) / 1e9;
    const double ms_cpu_por_segundo = double(cpu_us - antes_cpu_us_) / 1000.0 / segundos;
    antes_ns_ = ahora_ns;
    antes_cpu_us_ = cpu_us;
    return {Estado::kDisponible, segundos, ms_cpu_por_segundo};
  }

 private:
  int64_t antes_ns_ = 0;
  int64_t antes_cpu_us_ = 0;
  bool valida_ = false;
};

}  // namespace nfsmw::perfil_cpu
