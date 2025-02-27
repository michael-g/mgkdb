
//extern "C" int tpmux_main(int argc, char **argv);

namespace mg7x {
int tpmux_main(int argc, char **argv);
};

int main(int argc, char **argv)
{
	return mg7x::tpmux_main(argc, argv);
}
