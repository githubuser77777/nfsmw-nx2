#include "../../app/src/nfsmw_perfil_subetapas.h"

#include <cassert>

namespace {
int lecturas = 0;
int llamadas = 0;
}

int64_t nfsmw::hilos::CpuHiloUs(uint64_t manejador) {
  assert(manejador == 123);
  return ++lecturas * 10;
}

int main() {
  using namespace nfsmw::perfil_subetapas;
  Marcar();
  Medir<0>([] { ++llamadas; });
  assert(llamadas == 1 && lecturas == 0 && visitas == 0);
  manejador = 123;
  activa = true;
  Marcar();
  Medir<0>([] { ++llamadas; });
  Medir<1>([] { ++llamadas; });
  activa = false;
  assert(llamadas == 3 && lecturas == 6 && visitas == 1);
  assert(etapas[0].muestras == 1 && etapas[0].cpu_us == 10 && etapas[0].referencia_us == 10);
  assert(etapas[1].muestras == 1 && etapas[1].cpu_us == 10);
  MarcarVirtual();
  MedirVirtual<14>([] { ++llamadas; });
  assert(visitas_virtual == 0 && llamadas == 4 && lecturas == 6);
  virtual_activa = true;
  MarcarVirtual();
  MedirVirtual<14>([] { ++llamadas; });
  virtual_activa = false;
  assert(visitas_virtual == 1 && llamadas == 5 && lecturas == 9);
  assert(etapas_virtual[14].muestras == 1 && etapas_virtual[14].cpu_us == 10);
}
