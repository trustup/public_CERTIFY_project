#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>
#include <cmocka.h>
#include <Zp.h>


static void test_copies(void **state)
{
    char * seed="Seed_test_copies_0123456789";
    int seedLength=27;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z1=zpRandom(rng);
    Zp* z2=zpRandom(rng);
    Zp* z3=zpCopy(z1);
    assert_true(zpEquals(z1,z3));
    zpAdd(z3,z2);
    assert_false(zpEquals(z1,z3));
    zpAdd(z1,z2);
    assert_true(zpEquals(z1,z3));
    zpCopyValue(z3,z2);
    assert_true(zpEquals(z2,z3));
    zpAdd(z3,z1);
    assert_false(zpEquals(z2,z3));
    zpAdd(z2,z1);
    assert_true(zpEquals(z2,z3));
    zpFree(z1);
    zpFree(z2);
    zpFree(z3);
    rgFree(rng);
}

static void test_addition(void **state)
{
    char * seed="Seed_test_addition_0123456789";
    int seedLength=29;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z1=zpRandom(rng);
    Zp* z2=zpFromInt(12413);
    Zp* z3=zpFromInt(5403);
    Zp* z4=zpFromInt(5403);
    Zp* zRes=zpFromInt(17816);
    Zp* zero=zpFromInt(0);
    zpAdd(z2,z3);
    assert_true(zpEquals(z2,zRes)); //12413+5403=17816
    assert_true(zpEquals(z3,z4)); //z3 not modified
    zpAdd(zRes,z1);
    zpAdd(z1,z2);
    assert_true(zpEquals(z1,zRes)); 
    zpAdd(z1,zero);
    assert_true(zpEquals(z1,zRes)); 
    zpAdd(zero,z1);
    assert_true(zpEquals(z1,zero));
    zpFree(z1);
    zpFree(z2);
    zpFree(z3);
    zpFree(z4);
    zpFree(zRes);
    zpFree(zero);
    rgFree(rng);
}

static void test_neg_sub(void **state)
{
    char * seed="Seed_test_neg_sub_0123456789";
    int seedLength=28;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z1=zpRandom(rng);
    Zp* z2=zpCopy(z1);
    Zp* z3=zpFromInt(34);
    Zp* z4=zpFromInt(21);
    Zp* zRes=zpFromInt(13);
    zpNeg(z2);
    zpAdd(z2,z1);
    assert_true(zpIsZero(z2)); 
    zpSub(z3,z4);
    assert_true(zpEquals(z3,zRes)); 
    zpSub(z4,z4);
    assert_true(zpIsZero(z4)); 
    zpSub(z3,z4);
    assert_true(zpEquals(z3,zRes)); 
    zpFree(z1);
    zpFree(z2);
    zpFree(z3);
    zpFree(z4);
    zpFree(zRes);
    rgFree(rng);
}

static void test_multiplication(void **state)
{
    Zp* z1=zpFromInt(230);
    Zp* z2=zpFromInt(51);
    Zp* z3=zpCopy(z2);
    Zp* z4=zpFromInt(2);
    Zp* zRes=zpFromInt(11730);
    Zp* one=zpFromInt(1);
    zpMul(z1,z2);
    assert_true(zpEquals(z1,zRes));
    assert_true(zpEquals(z2,z3));
    zpCopyValue(z2,z1);
    zpMul(z1,z4);
    zpAdd(z2,z2);
    assert_true(zpEquals(z1,z2));
    zpMul(z1,one);
    assert_true(zpEquals(z1,z2));
    zpMul(one,z1);
    assert_true(zpEquals(z1,one));
    zpFree(z1);
    zpFree(z2);
    zpFree(z3);
    zpFree(z4);
    zpFree(one);
    zpFree(zRes);
}

static void test_serial(void **state){
    char * seed="Seed_test_serial_0123456789";
    int seedLength=27;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z1=zpRandom(rng);
    Zp* z2;
    int nbytes=zpByteSize();
    char *bytes1=malloc(nbytes*sizeof(char));
    zpToBytes(bytes1,z1);
    z2=zpFromBytes(bytes1);
    assert_true(zpEquals(z1,z2));
    zpFree(z2);
    bytes1[3]+=1;
    z2=zpFromBytes(bytes1);
    assert_false(zpEquals(z1,z2));
    free(bytes1);
    zpFree(z1);
    zpFree(z2);
    rgFree(rng);
}

static void test_bitops(void **state)
{
    Zp* z1=zpFromInt(27);
    Zp* z2=zpFromInt(54);
    Zp* z3=zpCopy(z1);
    Zp* zRes=zpFromInt(13);
    assert_int_equal(zpParity(z1),1);
    assert_int_equal(zpParity(z2),0);
    assert_int_equal(zpParity(zRes),1);
    assert_true(zpNbits(z1)==5);
    zpDouble(z1);
    assert_true(zpEquals(z1,z2));
    int reminder=zpHalf(z3);
    assert_int_equal(reminder,1);
    assert_true(zpEquals(z3,zRes));
    zpFree(z1);
    zpFree(z2);
    zpFree(z3);
    zpFree(zRes);
}

int main()
{
    const struct CMUnitTest zptests[] =
    {
        cmocka_unit_test(test_copies),
        cmocka_unit_test(test_addition),
        cmocka_unit_test(test_neg_sub),
        cmocka_unit_test(test_multiplication),
        cmocka_unit_test(test_serial),
        cmocka_unit_test(test_bitops)
    };
    //cmocka_set_message_output(CM_OUTPUT_XML);
	// Define environment variable CMOCKA_XML_FILE=testresults/libc.xml 
    return cmocka_run_group_tests(zptests, NULL, NULL);
}