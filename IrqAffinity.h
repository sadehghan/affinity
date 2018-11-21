#ifndef IRQ_AFFINITY_H_
#define IRQ_AFFINITY_H_

#include <iterator>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdio>
#include <stdlib.h>
#include <cstdint>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using std::string;
using std::vector;
using namespace boost::filesystem;

static const char* const PROC_IRQ_ADDRESS = "/proc/irq";
static const char* const SMP_AFFINITY_LIST_ADDRESS = "smp_affinity_list";
static const char* const DIGIT = "0123456789";

/**
 * A class for managing IRQ table (loading, getting, and setting IRQ names and affinities)
 * An IRQ (interrupt request) value is an assigned location where the computer can expect a particular device to
 * interrupt it when the device sends the computer signals about its operation.
 */
class IrqAffinity
{
public:
	typedef vector<uint32_t> CoreNumberSet;

	/// Constructs an invalid object on which you must call load_irq_info before using
	IrqAffinity()
	: is_valid(false)
	{
	}

	/**
	 * Loads devices name and IRQ numbers and stores them in the 'irq_info_set'
	 *
	 * @return true if successful, false otherwise
	 */
	bool load_irq_info()
	{
		// Check if info is already loaded
		if (is_valid)
			return true;

		path root(PROC_IRQ_ADDRESS);

		if (!exists(root) || !is_directory(root))
			return false;

		for (directory_iterator itr(root); itr != directory_iterator(); ++itr)
			if (is_directory(*itr))
			{
				string irq_number_string = path(*itr).filename().string();

				// If IRQ number is non-numeric
				if (irq_number_string.find_first_not_of(DIGIT) != string::npos)
					continue;
				int irq_number = atoi(irq_number_string.c_str());

				bool irq_has_name = false;
				for (directory_iterator sub_itr(itr->path()); sub_itr != directory_iterator(); ++sub_itr)
					if (is_directory(*sub_itr))
					{
						irq_has_name = true;
						IrqInfo insert_element;
						insert_element.name = path(*sub_itr).filename().string();
						insert_element.number = irq_number;
						irq_info_set.push_back(insert_element);
					}

				if (!irq_has_name)
				{
					IrqInfo insert_element;
					insert_element.number = irq_number;
					irq_info_set.push_back(insert_element);
				}
			}
		is_valid = true;
		return true;
	}

	/**
	 * Fills in 'irq_interface_name' by name of given IRQ number. It is possible that an IRQ number is valid but it has
	 * no name. In this case irq_interface_name will be empty and true is returned.
	 *
	 * @return true if successful, false otherwise
	 */
	bool get_interface_name_by_irq_number(const uint32_t irq_number, string& irq_interface_name) const
	{
		if (!is_valid)
			return false;

		for (const IrqInfo& irq_info : irq_info_set)
			if (irq_info.number == irq_number)
			{
				irq_interface_name = irq_info.name;
				return true;
			}

		return false;
	}

	/**
	 * Sets affinity of given IRQ number to given CPU core. Does it by writing to
	 * '/proc/irq/$irq_number/smp_affinity_list' file.
	 *
	 * @return true if successful, false otherwise
	 */
	bool set_affinity(const uint32_t irq_number, const CoreNumberSet& core_list) const
	{
		if (!is_valid)
			return false;

		string irq_id_address = PROC_IRQ_ADDRESS;
		irq_id_address += "/";
		irq_id_address += std::to_string(irq_number);
		irq_id_address += "/";
		irq_id_address += SMP_AFFINITY_LIST_ADDRESS;

		FILE* file_affinity = fopen(irq_id_address.c_str(), "w");
		if (!file_affinity)
			return false;

		const char* delimeter = "";
		for (uint32_t i : core_list)
		{
			if (fprintf(file_affinity, "%s%d", delimeter, i) < 0)
				return false;
			delimeter = ",";
		}

		if (fclose(file_affinity) == EOF)
			return false;

		return true;
	}

	/**
	 * Returns IRQ number of given name
	 *
	 * @param irq_name is name of IRQ which must be non-empty
	 *
	 * @return IRQ number if successful, -1 otherwise
	 */
	int get_irq_number(const string& irq_name) const
	{
		if (!is_valid || irq_name.empty())
			return -1;

		for (const IrqInfo& irq_info : irq_info_set)
			if (irq_info.name == irq_name)
				return irq_info.number;

		return -1;
	}

	/**
	 * Returns IRQ number of device name and queue number
	 *
	 * @param interface is name of network interface which must be non-empty
	 * @param queue is network card RX or TX queue number
	 *
	 * @return IRQ number if successful, -1 otherwise
	 */
	int get_irq_number(const string& interface, const uint32_t queue) const
	{
		if (!is_valid || interface.empty())
			return -1;

		for (const IrqInfo& irq_info : irq_info_set)
		{
			if (!boost::starts_with(irq_info.name, interface))
				continue;

			string sub_irq_name = irq_info.name.substr(interface.size(), irq_info.name.size() - interface.size());
			const size_t last_nondigit_index = sub_irq_name.find_last_not_of(DIGIT);

			if (string::npos == last_nondigit_index)
				continue;

			size_t last_digit_index = sub_irq_name.find_last_of(DIGIT);
			size_t first_digit_index = sub_irq_name.find_last_not_of(DIGIT, last_digit_index);
			string queue_number = sub_irq_name.substr(first_digit_index + 1, last_digit_index - first_digit_index);

			if (static_cast<uint32_t>(atoi(queue_number.c_str())) == queue)
			{
				return irq_info.number;
			}
		}

		return -1;
	}

	/**
	 * Returns IRQ number of device name and queue number and its direction (TX or RX)
	 *
	 * @param interface is name of network interface which must be non-empty
	 * @param queue is network card RX or TX queue number
	 * @param tx_wanted set to true if you want IRQ number of TX queue and set to false if you want IRQ number of RX
	 * queue.
	 *
	 * @return IRQ number of RX queue, or TX queue, or both (in case they share the same IRQ number) if successful,
	 * -1 otherwise
	 */
	int get_irq_number(const string& interface, const uint32_t queue, const bool tx_wanted) const
	{
		if (!is_valid)
			return -1;

		for (const IrqInfo& irq_info : irq_info_set)
		{
			if (!boost::starts_with(irq_info.name, interface))
				continue;

			string sub_irq_name = irq_info.name.substr(interface.size(), irq_info.name.size() - interface.size());
			const size_t last_nondigit_index = sub_irq_name.find_last_not_of(DIGIT);

			if (string::npos == last_nondigit_index)
				continue;

			size_t last_digit_index = sub_irq_name.find_last_of(DIGIT);
			size_t first_digit_index = sub_irq_name.find_last_not_of(DIGIT, last_digit_index);
			string queue_number = sub_irq_name.substr(first_digit_index + 1, last_digit_index - first_digit_index);

			if (static_cast<uint32_t>(atoi(queue_number.c_str())) == queue)
			{
				string interface_middle_name = sub_irq_name.substr(0, first_digit_index);
				boost::algorithm::to_lower(interface_middle_name);

				bool has_rx = boost::contains(interface_middle_name, "rx");
				bool has_tx = boost::contains(interface_middle_name, "tx");

				if (tx_wanted)
				{
					if (has_tx || (!has_rx && !has_tx))
						return irq_info.number;
				}
				else
				{
					if (has_rx || (!has_rx && !has_tx))
						return irq_info.number;
				}
			}
		}

		return -1;
	}

	CoreNumberSet get_all_irq_numbers_matching(const string& interface)
	{
		CoreNumberSet irq_number;

		for (const IrqInfo& irq_info : irq_info_set)
			if (boost::contains(irq_info.name, interface))
				irq_number.push_back(irq_info.number);

		return irq_number;
	}

private:
	/// A struct for storing device name and its IRQ number
	struct IrqInfo
	{
		string name;
		uint32_t number;
	};

	typedef vector<IrqInfo> IrqInfoSet;

	/// Shows that IRQ table successfully loaded
	bool is_valid;

	/// A vector for storing devices name and IRQ numbers
	IrqInfoSet irq_info_set;
};

#endif
