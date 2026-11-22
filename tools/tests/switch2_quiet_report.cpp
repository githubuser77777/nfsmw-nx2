#include "../../app/src/nfsmw_informe_silencioso.h"

#include <atomic>
#include <cassert>

int main() {
  std::atomic<int> counter{7};
  nfsmw::nativo::DescartarInforme("{}", counter.exchange(0));
  assert(counter.load() == 0);
}
