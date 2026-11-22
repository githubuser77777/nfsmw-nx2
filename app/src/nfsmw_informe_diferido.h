#pragma once
/*
 * Periodic log lines, off the working threads.
 *
 * The log is synchronous (log_async = false): every few lines the FILE is flushed to the SD card and the
 * writing thread stalls for several ms. Every 10 s the game thread used to write 16 lines, the audio thread 3
 * and the ring thread 7 (the C2 lines of nfsmw_nativo_destinos.cpp) on their own. These lines are formatted
 * on the thread that produces them and written by the "NFSMW informes" thread (InformeDiferido,
 * nfsmw_nativo_sistema.cpp). Warnings and errors are still written immediately, from their own thread.
 */
#include <string>

#include <rex/logging.h>
#include "nfsmw_informe_silencioso.h"

namespace nfsmw::nativo {
void InformeDiferido(std::string linea);
}  // namespace nfsmw::nativo

#if defined(__SWITCH2__) && !defined(NFSMW_SWITCH2_DIAGNOSTICS)
#define NFSMW_INFORME_DIFERIDO(...) ::nfsmw::nativo::DescartarInforme(__VA_ARGS__)
#else
#define NFSMW_INFORME_DIFERIDO(...) ::nfsmw::nativo::InformeDiferido(fmt::format(__VA_ARGS__))
#endif
