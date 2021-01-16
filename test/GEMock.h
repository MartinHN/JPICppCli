struct GEMock {
  template <typename T> T echo(T f) { return f; }
  void echovoid() {}
  template <typename... Args> void trigWithParams(Args... args){};
  float mf = 1;
};

struct MockAPI : public API<GEMock> {

  MockAPI() {
    rFunction<float, float>("echof", &GEMock::echo<float>);
    rFunction<int, int>("echoi", &GEMock::echo<int>);
    rFunction<double, double>("echod", &GEMock::echo<double>);
    // rFunction<void, void>("echovoid", &GEMock::echovoid);
    // rFunction<void>("echovoidNoArg", &GEMock::echovoid);
    rFunction<void, int>("trigWithParami", &GEMock::trigWithParams<int>);
    rFunction<void, int, int>("trigWithParamii",
                              &GEMock::trigWithParams<int, int>);
    // rFunction<void, int, float>("trigWithParamif",
    //                             &GEMock::trigWithParams<int, float>);
    rTrig("echovoidNoArg", &GEMock::echovoid);
    rMember("mf", &GEMock::mf);
  }
};

/////////////////
// TESTS
#include "gtest/gtest.h"
MockAPI api;
GEMock inst;


template <typename T> void testType(const std::string &fname, double val) {
  FunctionOfInstance<GEMock> *f = api.getFunction(fname);
  ASSERT_TRUE(f != nullptr);
  T testVal(val);
  auto res = f->call(&inst, {testVal});
  ASSERT_TRUE(res != nullptr);
  EXPECT_TRUE(res->template is<T>());
  EXPECT_TRUE(res->template get<T>() == testVal);
}

template <typename T>
void testTrigWithParam(const std::string &fname, double val) {
  FunctionOfInstance<GEMock> *f = api.getFunction(fname);
  ASSERT_TRUE(f != nullptr);
  T testVal(val);
  auto res = f->call(&inst, {testVal});
  ASSERT_TRUE(res != nullptr);
  EXPECT_TRUE(res->template is<void>());
}

template <typename T, typename T2>
void testTrigWithParam(const std::string &fname, double val, double val2) {
  FunctionOfInstance<GEMock> *f = api.getFunction(fname);
  ASSERT_TRUE(f != nullptr);
  T testVal(val);
  T2 testVal2(val2);
  auto res = f->call(&inst, {testVal, testVal2});
  ASSERT_TRUE(res != nullptr);
  EXPECT_TRUE(res->template is<void>());
  auto types = f->getArgTypes();
  ASSERT_TRUE(types.size() == 2);
  EXPECT_TRUE(types[0] == TypeOf<T>::name());
  EXPECT_TRUE(types[1] == TypeOf<T2>::name());
}

void testTrig(const std::string &fname) {
  FunctionOfInstance<GEMock> *f = api.getFunction(fname);
  ASSERT_TRUE(f != nullptr);
  auto res = f->call(&inst, {});
  ASSERT_TRUE(res != nullptr);
  EXPECT_TRUE(res->is<void>());
}

void testEchoAllTypes(double val) {
  testType<float>("echof", val);
  testType<int>("echoi", val);
  testType<double>("echod", val);
}

TEST(APITest, canFetch) {
  ASSERT_TRUE(api.getFunction("echof") != nullptr);
  ASSERT_TRUE(api.getFunction("echofoo") == nullptr);
}

TEST(APITest, canCall) {

  testEchoAllTypes(0);
  testEchoAllTypes(1);
  testTrig("echovoidNoArg");
  testTrigWithParam<int>("trigWithParami", 1);
  // testTrigWithParam<int, float>("trigWithParamif", 1, 1.f);
  testTrigWithParam<int, int>("trigWithParamii", 1, 1);
}

TEST(APITest, toString) {

  // APIInstance<GEMock> apiInst(inst, api);
  // auto apiState = apiInst.toString();
  // EXPECT_TRUE(apiState.size() > 0);
  // PRINTLN(apiState);
  // bool success = apiInst.fromString(apiState);
  // EXPECT_TRUE(success);
  // auto apiState2 = apiInst.toString();
  // EXPECT_TRUE(apiState2 == apiState);
}

TEST(APITest, Serializer) {
  APIInstance<GEMock> apiInst(&inst, &api);
  auto state = APISerializer::apiStateToString(&apiInst);
  EXPECT_TRUE(state.size());
}

TEST(APITest, Singleton) {
  struct S : public APIAndInstance<S> {
    S() : APIAndInstance<S>(this) { rMember<float>("l", &S::l); }
    float l = 0;
  };
  S s;

  auto state = APISerializer::apiStateToString(&s);
  PRINTLN(state.c_str());
  EXPECT_TRUE(state.size());
}

TEST(APITest, SerializerNode) {
  struct S2 : public APIAndInstance<S2>, public MapNode {
    S2() : APIAndInstance<S2>(this) {
      rMember<int>("l2", &S2::l2, {{"min", "0"}});
    }
    int l2 = 2;
  };
  S2 s2;
  struct S : public APIAndInstance<S>, public MapNode {
    S() : APIAndInstance<S>(this) {
      rMember<float>("l", &S::l);
      addChild("this", this);

      PRINTLN("inited");
    }
    float l = 10;
  };
  S s;
  s.addChild("s2", &s2);
  s2.addChild("sparent", &s);
  auto schema = APISerializer::schemaFromNode(&s);
  PRINTLN(schema.c_str());
  EXPECT_TRUE(schema.size());
  auto state = APISerializer::stateFromNode(&s);
  EXPECT_TRUE(state.size());

  APISerializer::stateToNode(&s, state);
  auto state2 = APISerializer::stateFromNode(&s);
  EXPECT_TRUE(state == state2);
  s.l = 9;
  state2 = APISerializer::stateFromNode(&s);
  EXPECT_TRUE(state != state2);
  PRINTLN(state2);
}
