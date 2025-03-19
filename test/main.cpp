#define BOOST_TEST_MODULE RULANG_TEST
#include<boost/test/unit_test.hpp>
/*
boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[])
{
	return 0;
}
*/
int cpp_main(int argc, char* argv[])
{
	return boost::unit_test_framework::unit_test_main(init_unit_test_suite, argc, argv);
}