#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>
#include <cmocka.h>
#include <Zp.h>
#include <g1.h>


static void test_copies(void **state)
{
    char * seed="Seed_test_copies_0123456789";
    int seedLength=27;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z=zpRandom(rng);
    G1* g1=g1Generator();
    g1Mul(g1,z);
    G1* g2=g1Copy(g1);
    G1* g3=g1Generator();
    zpAdd(z,z);
    g1Mul(g3,z);
    assert_true(g1Equals(g1,g2));
    g1Add(g1,g3);
    assert_false(g1Equals(g1,g2));
    g1Add(g2,g3);
    assert_true(g1Equals(g1,g2));
    g1CopyValue(g2,g3);
    assert_true(g1Equals(g2,g3));
    g1Add(g2,g1);
    assert_false(g1Equals(g2,g3));
    g1Add(g3,g1);
    assert_true(g1Equals(g2,g3));
    g1Free(g1);
    g1Free(g2);
    g1Free(g3);
    zpFree(z);
    rgFree(rng);
}

static void test_addition(void **state) //TODO Test using precomputed elements.
{
    char * seed="Seed_test_addition_0123456789";
    int seedLength=29;
    ranGen * rng=rgInit(seed,seedLength);
    G1* identity=g1Identity();
    Zp* z1=zpRandom(rng);
    Zp* z2=zpRandom(rng);
    G1* g1=g1Generator(); //g1=[z1]g
    g1Mul(g1,z1); 
    G1* g2=g1Generator(); //g2=[z2]g
    g1Mul(g2,z2);
    G1* g3=g1Generator();
    zpAdd(z1,z2);
    g1Mul(g3,z1);          //g3=[z1+z2]g
    G1* g4=g1Copy(g2);
    g1Add(g1,g2);
    assert_true(g1Equals(g1,g3)); //Sum is fine
    assert_true(g1Equals(g2,g4)); //Second operand not modified
    g1Add(g1,g2);
    g1Add(g2,g3);
    assert_true(g1Equals(g1,g2)); 
    g1Add(g1,identity);
    assert_true(g1Equals(g1,g2)); 
    g1Add(identity,g1);
    assert_true(g1Equals(g1,identity)); 
    g1Free(g1);
    g1Free(g2);
    g1Free(g3);
    g1Free(g4);
    g1Free(identity);
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
    G1* g1=g1Generator(); 
    G1* g2=g1Generator(); 
    G1* g=g1Generator(); 
    g1Add(g1,g1);
    g1Mul(g2,z1);
    assert_true(g1Equals(g1,g2)); //g+g=[2]g
    g1Add(g1,g);
    g1CopyValue(g2,g);
    g1Mul(g2,z2);
    assert_true(g1Equals(g1,g2)); //g+g+g=[3]g
    g1CopyValue(g1,g);
    g1CopyValue(g2,g);
    zpRandomValue(rng,z1);
    g1Mul(g1,z1);
    zpAdd(z1,one);
    g1Mul(g2,z1);
    g1Add(g1,g);
    assert_true(g1Equals(g1,g2)); //[r]g+g=[r+1]g
    g1Free(g1);
    g1Free(g2);
    g1Free(g);
    zpFree(z1);
    zpFree(z2);
}

static void test_serial(void **state){
    char * seed="Seed_test_serial_0123456789";
    int seedLength=27;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z=zpRandom(rng);
    G1* g1=g1Generator();
    G1* g2;
    int nbytes=g1ByteSize();
    char *bytes1=malloc(nbytes*sizeof(char));
    g1ToBytes(bytes1,g1);
    g2=g1FromBytes(bytes1);
    assert_true(g1Equals(g1,g2));
    g1Free(g2);
    bytes1[3]+=1;
    g2=g1FromBytes(bytes1);
    assert_false(g1Equals(g1,g2));
    free(bytes1);
    zpFree(z);
    g1Free(g1);
    g1Free(g2);
    rgFree(rng);
}

static void test_muln(void **state){
    char * seed="Seed_test_muln_0123456789";
    int seedLength=25;
    ranGen * rng=rgInit(seed,seedLength);
    int n=20;
    G1 *res1, *res2;
    G1 *auxG1=g1Generator();
    Zp *auxZp=zpFromInt(0);
    G1 **bases=malloc(n*sizeof(G1*));
    Zp **scalars=malloc(n*sizeof(Zp*));
    for(int i=0;i<n;i++){
        scalars[i]=zpRandom(rng);
        bases[i]=g1Generator();
        zpRandomValue(rng,auxZp);
        g1Mul(bases[i],auxZp);
    }
    res1=g1Copy(bases[0]);
    g1Mul(res1,scalars[0]);
    for(int i=1;i<n;i++){
        g1CopyValue(auxG1,bases[i]); 
        g1Mul(auxG1,scalars[i]);
        g1Add(res1,auxG1);
    } 
    res2=g1Muln(bases,scalars,n);
    assert_true(g1Equals(res1,res2));
    g1Free(res1);
    g1Free(res2);
    g1Free(auxG1);
    zpFree(auxZp);
    for(int i=0;i<n;i++){
        g1Free(bases[i]);
        zpFree(scalars[i]);
    }
    free(bases);
    free(scalars);
    rgFree(rng);
}

static void test_mul_lookup(void **state)
{
    char * seed="Seed_test_mul_lookup_0123456789";
    int seedLength=31;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z1=zpRandom(rng);
    G1* g=g1Generator(); 
    G1* g2=g1Generator();
    g1Add(g2,g2);
    G1* res1;
    int n=256;
    G1** lt=g1CompLookupTable(g,n);
    res1=g1MulLookup(lt,z1);
    g1Mul(g,z1);
    assert_true(g1Equals(g,res1));
    g1Free(g);
    zpFree(z1);
    g1Free(res1);
    for(int i=0;i<n;i++)
        g1Free(lt[i]);
    free(lt);
}

int main()
{
    const struct CMUnitTest g1tests[] =
    {
        cmocka_unit_test(test_copies),
        cmocka_unit_test(test_addition),
        cmocka_unit_test(test_multiplication),
        cmocka_unit_test(test_muln),
        cmocka_unit_test(test_serial),
        cmocka_unit_test(test_mul_lookup)
    };
    //cmocka_set_message_output(CM_OUTPUT_XML);
	// Define environment variable CMOCKA_XML_FILE=testresults/libc.xml 
    return cmocka_run_group_tests(g1tests, NULL, NULL);
}