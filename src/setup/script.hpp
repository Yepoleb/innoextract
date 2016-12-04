#ifndef INNOEXTRACT_SETUP_SCRIPT_HPP
#define INNOEXTRACT_SETUP_SCRIPT_HPP

#include "util/fstream.hpp"
#include "setup/info.hpp"

namespace setup {

void write_script(util::ofstream & ofs, const setup::info & info);

} // namespace setup

#endif // INNOEXTRACT_SETUP_SCRIPT_HPP
