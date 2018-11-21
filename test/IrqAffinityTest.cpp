#include <iostream>
#include <string>

#include "IrqAffinity.h"

using namespace std;

int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		cerr << "Error: No network interface name provided\n";
		cerr << "Usage: " << argv[0] << " <if-name> [queue-number] [want tx ? true]\n";
		return 1;
	}

	IrqAffinity irq_affinity;

	if (!irq_affinity.load_irq_info())
	{
		cerr << "Error: Failed to load IRQ information from " << PROC_IRQ_ADDRESS << '\n';
		return 1;
	}

	int irq_number = -1;
	if (2 == argc)
		irq_number = irq_affinity.get_irq_number(argv[1]);
	else if (3 == argc)
		irq_number = irq_affinity.get_irq_number(argv[1], atoi(argv[2]));
	else if (4 == argc)
	{
		std::string tx_wanted_str(argv[3]);
		bool tx_wanted = (tx_wanted_str == "true") ? true : false;
		irq_number = irq_affinity.get_irq_number(argv[1], atoi(argv[2]), tx_wanted);
	}

	if (irq_number < 0)
	{
		cerr << "Error: Failed to find IRQ number of given device\n";
		return 1;
	}

	cerr << "Info: IRQ number of given device is " << irq_number << '\n';

	IrqAffinity::CoreNumberSet number { 0, 5, 6 };

	if (!irq_affinity.set_affinity(irq_number, number))
	{
		cerr << "Error: Failed to set IRQ affinity on given device\n";
		return 1;
	}

	cerr << "Info: Successfully set IRQ affinity on given device\n";

	return 0;
}
