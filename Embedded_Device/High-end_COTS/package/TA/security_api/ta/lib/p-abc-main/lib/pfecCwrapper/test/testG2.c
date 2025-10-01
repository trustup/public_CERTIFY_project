#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>
#include <cmocka.h>
#include <Zp.h>
#include <g2.h>


static void test_copies(void **state)
{
    char * seed="Seed_test_copies_0123456789";
    int seedLength=27;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z=zpRandom(rng);
    G2* g1=g2Generator();
    g2Mul(g1,z);
    G2* g2=g2Copy(g1);
    G2* g3=g2Generator();
    zpAdd(z,z);
    g2Mul(g3,z);
    assert_true(g2Equals(g1,g2));
    g2Add(g1,g3);
    assert_false(g2Equals(g1,g2));
    g2Add(g2,g3);
    assert_true(g2Equals(g1,g2));
    g2CopyValue(g2,g3);
    assert_true(g2Equals(g2,g3));
    g2Add(g2,g1);
    assert_false(g2Equals(g2,g3));
    g2Add(g3,g1);
    assert_true(g2Equals(g2,g3));
    g2Free(g1);
    g2Free(g2);
    g2Free(g3);
    zpFree(z);
    rgFree(rng);
}

static void test_addition(void **state) //TODO Test using precomputed elements. 
{
    char * seed="Seed_test_addition_0123456789";
    int seedLength=29;
    ranGen * rng=rgInit(seed,seedLength);
    G2* identity=g2Identity();
    Zp* z1=zpRandom(rng);
    Zp* z2=zpRandom(rng);
    G2* g1=g2Generator(); //g1=[z1]g
    g2Mul(g1,z1); 
    G2* g2=g2Generator(); //g2=[z2]g
    g2Mul(g2,z2);
    G2* g3=g2Generator();
    zpAdd(z1,z2);
    g2Mul(g3,z1);          //g3=[z1+z2]g
    G2* g4=g2Copy(g2);
    g2Add(g1,g2);
    assert_true(g2Equals(g1,g3)); //Sum is fine
    assert_true(g2Equals(g2,g4)); //Second operand not modified
    g2Add(g1,g2);
    g2Add(g2,g3);
    assert_true(g2Equals(g1,g2)); 
    g2Add(g1,identity);
    assert_true(g2Equals(g1,g2));
    g2Add(identity,g1);
    assert_true(g2Equals(g1,identity));
    g2Free(g1);
    g2Free(g2);
    g2Free(g3);
    g2Free(g4);
    zpFree(z1);
    zpFree(z2);
    rgFree(rng);
}



static void test_multiplication(void **state)
{
    char * seed="Seed_test_multiplication_0123456789";
    int seedLength=35;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* one=zpFromInt(1);
    Zp* z1=zpFromInt(2);
    Zp* z2=zpFromInt(3);
    G2* g1=g2Generator(); 
    G2* g2=g2Generator(); 
    G2* g=g2Generator(); 
    g2Add(g1,g1);
    g2Mul(g2,z1);
    assert_true(g2Equals(g1,g2)); //g+g=[2]g
    g2Add(g1,g);
    g2CopyValue(g2,g);
    g2Mul(g2,z2);
    assert_true(g2Equals(g1,g2)); //g+g+g=[3]g
    g2CopyValue(g1,g);
    g2CopyValue(g2,g);
    zpRandomValue(rng,z1);
    g2Mul(g1,z1);
    zpAdd(z1,one);
    g2Mul(g2,z1);
    g2Add(g1,g);
    assert_true(g2Equals(g1,g2)); //[r]g+g=[r+1]g
    g2Free(g1);
    g2Free(g2);
    g2Free(g);
    zpFree(z1);
    zpFree(z2);
}

static void test_serial(void **state){
    char * seed="Seed_test_serial_0123456789";
    int seedLength=27;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z=zpRandom(rng);
    G2* g1=g2Generator();
    G2* g2;
    int nbytes=g2ByteSize();
    char *bytes1=malloc(nbytes*sizeof(char));
    g2ToBytes(bytes1,g1);
    g2=g2FromBytes(bytes1);
    assert_true(g2Equals(g1,g2));
    g2Free(g2);
    bytes1[3]+=1;
    g2=g2FromBytes(bytes1);
    assert_false(g2Equals(g1,g2));
    free(bytes1);
    zpFree(z);
    g2Free(g1);
    g2Free(g2);
    rgFree(rng);
}

int main()
{
    const struct CMUnitTest g2tests[] =
    {
        cmocka_unit_test(test_copies),
        cmocka_unit_test(test_addition),
        cmocka_unit_test(test_multiplication),
        cmocka_unit_test(test_serial)
    };
    //cmocka_set_message_output(CM_OUTPUT_XML);
	// Define environment variable CMOCKA_XML_FILE=testresults/libc.xml 
    return cmocka_run_group_tests(g2tests, NULL, NULL);
}