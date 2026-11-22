#include "../../app/src/nfsmw_perfil_cpu.h"

#include <cassert>
#include <cmath>

int main() {
  using namespace nfsmw::perfil_cpu;
  Ventana ventana;
  assert(ventana.Actualizar(1000000000, 100000).estado == Estado::kInicio);
  const Muestra normal = ventana.Actualizar(11000000000, 7600000);
  assert(normal.estado == Estado::kDisponible);
  assert(std::abs(normal.segundos - 10.0) < 1e-9);
  assert(std::abs(normal.ms_cpu_por_segundo - 750.0) < 1e-9);
  assert(ventana.Actualizar(12000000000, -1).estado == Estado::kNoDisponible);
  assert(ventana.Actualizar(13000000000, 8000000).estado == Estado::kInicio);
  assert(ventana.Actualizar(13000000000, 8100000).estado == Estado::kInicio);
  assert(ventana.Actualizar(14000000000, 100).estado == Estado::kInicio);
  const Muestra recuperada = ventana.Actualizar(15000000000, 250100);
  assert(recuperada.estado == Estado::kDisponible);
  assert(std::abs(recuperada.ms_cpu_por_segundo - 250.0) < 1e-9);

  Etapa etapa;
  etapa.Agregar(100, 102, 110);
  etapa.Agregar(110, 111, 115);
  etapa.Agregar(-1, 0, 1);
  etapa.Agregar(10, 9, 11);
  etapa.Agregar(10, 11, 10);
  assert(etapa.muestras == 2);
  assert(etapa.fallos == 3);
  assert(etapa.referencia_us == 3);
  assert(etapa.cpu_us == 12);

  Destinos<2> destinos;
  destinos.Agregar(0, 100, 102, 110);
  destinos.Agregar(42, -1, 0, 1);
  destinos.Agregar(99, 1, 2, 3);
  destinos.Agregar(0, 110, 111, 115);
  destinos.Agregar(42, 5, 6, 9);
  assert(destinos.usados == 2);
  assert(destinos.descartados == 1);
  assert(destinos.entradas[0].destino == 0);
  assert(destinos.entradas[0].etapa.muestras == 2);
  assert(destinos.entradas[0].etapa.cpu_us == 12);
  assert(destinos.entradas[0].etapa.referencia_us == 3);
  assert(destinos.entradas[1].destino == 42);
  assert(destinos.entradas[1].etapa.muestras == 1);
  assert(destinos.entradas[1].etapa.fallos == 1);
}
