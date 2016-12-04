#include <iostream>

#include <boost/foreach.hpp>

#include "setup/version.hpp"
#include "setup/info.hpp"
#include "setup/script.hpp"
#include "setup/item.hpp"
#include "setup/windows.hpp"
#include "setup/registry.hpp"

#include "util/fstream.hpp"
#include "util/output.hpp"

namespace setup {

struct keyvalue {

	const std::string & key;
	const std::string & value;

	explicit keyvalue(const std::string & _key, const std::string & _value)
		: key(_key), value(_value) { }

};

static std::ostream & operator<<(std::ostream & os, const keyvalue & kv) {
	if (!kv.value.empty()) {
		return os << kv.key << "=" << kv.value << "\n";
	} else {
		return os;
	}
}

namespace detail {

template <class T>
struct property {

	const std::string & key;
	const T value;
	const bool first;

	explicit property(const std::string & _key, const T _value,
	                  bool _first=false)
		: key(_key), value(_value), first(_first) { }

};

template <class T>
std::ostream & operator<<(std::ostream & os, const property<T> & prop) {
	if (!prop.first) {
		os << "; ";
	}
	return os << prop.key << ": " << prop.value;
}

template <>
std::ostream & operator<<(std::ostream & os, const property<std::string> & prop) {
	if (!prop.value.empty()) {
		if (!prop.first) {
			os << "; ";
		}
		return os << prop.key << ": " << prop.value;
	} else {
		return os;
	}
}

}

template <class T>
detail::property<T> property(const std::string & key, const T value, bool first=false) {
	return detail::property<T>(key, value, first);
}

struct quoted_nocolor {

	const std::string & str;

	explicit quoted_nocolor(const std::string & _str) : str(_str) { }

};

inline std::ostream & operator<<(std::ostream & os, const quoted_nocolor & q) {
	os << '"';
	for(std::string::const_iterator i = q.str.begin(); i != q.str.end(); ++i) {
		unsigned char c = (unsigned char)*i;
		if(c < ' ' && c != '\t' && c != '\r' && c != '\n') {
			std::ios_base::fmtflags old = os.flags();
			os << '<' << std::hex << std::setfill('0') << std::setw(2)
			   << int(c) << '>';
			os.setf(old, std::ios_base::basefield);
		} else {
			os << *i;
		}
	}
	return os << '"';
}

template <class T>
std::string stringify(const T& value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

static void write_version(util::ofstream & ofs, const setup::version & version) {
	ofs << ";InnoSetupVersion=";
	ofs << version.a() << "." << version.b() << "." << version.c();
	if (version.unicode) {
		ofs << " (Unicode)";
	}
	ofs << "\n";
}

static void write_setup(util::ofstream & ofs, const setup::header & header) {
	ofs << "\n[Setup]\n";
	ofs << keyvalue("AppName", header.app_name);
	ofs << keyvalue("AppComments", header.app_comments);
	ofs << keyvalue("AppContact", header.app_contact);
	ofs << keyvalue("AppCopyright", header.app_copyright);
	ofs << keyvalue("AppId", header.app_id);
	ofs << keyvalue("AppModifyPath", header.app_modify_path);
	ofs << keyvalue("AppMutex", header.app_mutex);
	ofs << keyvalue("AppName", header.app_name);
	ofs << keyvalue("AppPublisher", header.app_publisher);
	ofs << keyvalue("AppPublisherUrl", header.app_publisher_url);
	ofs << keyvalue("AppReadmeFile", header.app_readme_file);
	ofs << keyvalue("AppSupportPhone", header.app_support_phone);
	ofs << keyvalue("AppSupportUrl", header.app_support_url);
	ofs << keyvalue("AppUpdatesUrl", header.app_updates_url);
	ofs << keyvalue("AppVersion", header.app_version);
	ofs << keyvalue("AppVerName", header.app_versioned_name);
	ofs << keyvalue("OutputBaseFilename", header.base_filename);
	ofs << keyvalue("CloseApplicationsFilter", header.close_applications_filter);
	//ofs << keyvalue("CompiledCode", header.compiled_code);
	ofs << keyvalue("CreateUninstallRegKey", header.create_uninstall_registry_key);
	ofs << keyvalue("DefaultDirName", header.default_dir_name);
	ofs << keyvalue("DefaultGroupName", header.default_group_name);
	ofs << keyvalue("DefaultUserInfoSerial", header.default_serial);
	ofs << keyvalue("DefaultUserInfoName", header.default_user_name);
	ofs << keyvalue("DefaultUserInfoOrg", header.default_user_organisation);
	ofs << keyvalue("InfoAfterFile", header.info_after);
	ofs << keyvalue("InfoBeforeFile", header.info_before);
	// TODO: Export text to file
	//ofs << keyvalue("LicenseFile", header.license_text);
	ofs << keyvalue("SetupMutex", header.setup_mutex);
	ofs << keyvalue("UninstallFilesDir", header.uninstall_files_dir);
	ofs << keyvalue("UninstallIconFile", header.uninstall_icon);
	ofs << keyvalue("UninstallIconName", header.uninstall_icon_name);
	ofs << keyvalue("UninstallDisplayName", header.uninstall_name);
	ofs << keyvalue("Uninstallable", header.uninstallable);
	//ofs << keyvalue("UninstallerSignature", header.uninstaller_signature);
}

void write_item(util::ofstream & ofs, const setup::item & item, const setup::header & header)
{
	const setup::windows_version_range & default_winver = header.winver;

	ofs << property("Languages", item.languages);
	// TODO: Make sure version formatting is correct
	if (item.winver.begin != default_winver.begin) {
		ofs << property("MinVersion", item.winver.begin.win_version);
	}
	if (item.winver.end != default_winver.end) {
		ofs << property("OnlyBelowVersion", item.winver.end.win_version);
	}
	ofs << property("Components", item.components);
	ofs << property("Tasks", item.tasks);
}

void write_registry(util::ofstream & ofs, const setup::info & info)
{
	ofs << "\n[Registry]\n";
	std::cout << info.registry_entries.size() << std::endl;
	BOOST_FOREACH(const setup::registry_entry & entry, info.registry_entries) {
		// TODO: Make stringify obsolete
		ofs << property("Root", stringify(entry.hive), true);
		ofs << property("Subkey", quoted_nocolor(entry.key));
		if(entry.type != setup::registry_entry::None) {
			ofs << property("ValueType", stringify(entry.type));
		}
		if (!entry.name.empty()) {
			ofs << property("ValueName", quoted_nocolor(entry.name));
		}
		if (!entry.value.empty()) {
			ofs << property("ValueData", quoted_nocolor(entry.value));
		}
		// Permissions, Components, Flags
		if (entry.permission != -1) {
			ofs << property("Permissions", entry.permission);
		}
		write_item(ofs, entry, info.header);
		// TODO: Format correctly
		if (entry.options != 0) {
			ofs << property("Flags", entry.options);
		}
		ofs << "\n";
	}
}

void write_script(util::ofstream & ofs, const setup::info & info) {
	write_version(ofs, info.version);

	write_setup(ofs, info.header);
	write_registry(ofs, info);
}

} // namespace setup
