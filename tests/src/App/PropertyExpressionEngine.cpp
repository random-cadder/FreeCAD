#include "gtest/gtest.h"

#include <Base/Quantity.h>

#include "App/Application.h"
#include "App/Document.h"
#include "App/DocumentObject.h"
#include <App/ObjectIdentifier.h>
#include <App/Expression.h>
#include "App/PropertyExpressionEngine.h"

#include <src/App/InitApplication.h>

#include <typeinfo>

// clang-format off

class PropertyExpressionEngine: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _doc_name = App::GetApplication().getUniqueDocumentName("test");
        _this_doc = App::GetApplication().newDocument(_doc_name.c_str(), "testUser");
        _this_obj = _this_doc -> addObject("Sketcher::SketchObject");
        _source_name = std::string("this_origin");
        _source_prop = _this_obj -> addDynamicProperty("App::PropertyString", _source_name.c_str()); // property with length as string
        _target_name = std::string("this_length");
        _target_prop = _this_obj -> addDynamicProperty("App::PropertyLength", _target_name.c_str()); // property with reference to source
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_doc_name.c_str());
    }

    std::string doc_name() { return _doc_name; }
    App::Document* this_doc() { return _this_doc; }
    App::DocumentObject* this_obj() { return _this_obj; }
    std::string source_name() { return _source_name; }
    App::Property* source_prop() { return _source_prop; }
    std::string target_name() { return _target_name; }
    App::Property* target_prop() { return _target_prop; }

private:
    std::string _doc_name;
    App::Document* _this_doc {};
    App::DocumentObject* _this_obj {};
    std::string _source_name;
    App::Property* _source_prop {};
    std::string _target_name;
    App::Property* _target_prop {};
};

TEST_F(PropertyExpressionEngine, execute)
{
    try {
        auto source_text = std::string("1.5 m"); // provided in source
        auto target_text = std::string("1500 mm"); // expected in target

        auto source_path = App::ObjectIdentifier::parse(this_obj(), source_name());
        source_prop()->setPathValue(source_path, source_text);

        auto target_path = App::ObjectIdentifier::parse(this_obj(), target_name());
        std::shared_ptr<App::Expression> target_rule(App::Expression::parse(this_obj(), source_name()));
        this_obj()->setExpression(target_path, target_rule);

        this_obj() -> ExpressionEngine.execute();

        auto source_entry = source_prop() -> getPathValue(source_path);
        ASSERT_TRUE(source_entry.type() == typeid(std::string));
        auto source_value = App::any_cast<std::string>(source_entry);
        // sample output: source_value=1.5 m
        std::cerr << "### PASS_1 source_value=" << source_value << std::endl;

        auto target_entry = target_prop() -> getPathValue(target_path);
        ASSERT_TRUE(target_entry.type() == typeid(Base::Quantity));
        auto target_quant = App::any_cast<Base::Quantity>(target_entry);
        auto target_value = target_quant.getValue();
        auto target_unit = target_quant.getUnit().getString().toStdString();
        // sample output: target_value=1.5 target_unit=mm
        std::cerr << "### PASS_2 target_value=" << target_value << " target_unit=" << target_unit << std::endl;

        // failure point:
        EXPECT_EQ(target_quant, App::parseQuantityFromText(target_text)) << \
            "expecting as equal: source_text='" + source_text + "' target_text='" + target_text + "'";
    }
    catch(Base::Exception const & error) {
        FAIL() << "### FAIL_1: " << error.what();
    }
    catch(std::exception const & error) {
        FAIL() << "### FAIL_2: " << error.what();
    }
    catch(...) {
        FAIL() << "### FAIL_3: BADA-BOOM";
    }
}

// clang-format on
