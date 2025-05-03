#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H

#include <llvm-18/llvm/ADT/APFloat.h>
#include <llvm-18/llvm/ADT/STLExtras.h>
#include <llvm-18/llvm/IR/BasicBlock.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DerivedTypes.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Module.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Verifier.h>
#include <llvm-18/llvm/ADT/StringRef.h>
#include <llvm-18/llvm/Support/raw_ostream.h>
#include <string>
#include <utility>
#include <vector>
#include <memory>

llvm::Value *logErrorV(const char *str);

//Codegen
static std::unique_ptr<llvm::LLVMContext> m_context;
static std::unique_ptr<llvm::Module> m_module;
static std::unique_ptr<llvm::IRBuilder<>> m_builder;
static std::map<std::string, llvm::Value *> m_named_values;

class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value* codegen() = 0;
};

class NumberExprAST : public ExprAST {
public:
    explicit NumberExprAST(const double value) : m_value(value) {}
    llvm::Value* codegen() override {
        return llvm::ConstantFP::get(*m_context, llvm::APFloat(m_value));
    }
private:
    double m_value;
};

class VariableExprAST : public ExprAST {
public:
    explicit VariableExprAST(std::string  name) : m_name(std::move(name)) {}
    llvm::Value* codegen() override {
        llvm::Value* v = m_named_values[m_name];
        if (!v)
            return logErrorV("Unknown variable name");
        return v;
    }
private:
    std::string m_name;
};

class BinaryExprAST : public ExprAST {
public:
    explicit BinaryExprAST(const char op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : m_op(op), m_LHS(std::move(LHS)), m_RHS(std::move(RHS)) {}
    llvm::Value* codegen() override {
        llvm::Value *L = m_LHS->codegen();
        llvm::Value *R = m_RHS->codegen();
        if (!L || !R)
            return nullptr;

        switch (m_op) {
            case '+':
                return m_builder->CreateFAdd(L, R, "addtmp");
            case '-':
                return m_builder->CreateFSub(L, R, "subtmp");
            case '*':
                return m_builder->CreateFMul(L, R, "multmp");
            case '<':
                L = m_builder->CreateFCmpULT(L, R, "cmptmp");
                // Convert bool 0/1 to double 0.0 or 1.0
                return m_builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*m_context), "booltmp");
            default:
                return logErrorV("invalid binary operator");
        }
    }
private:
    char m_op;
    std::unique_ptr<ExprAST> m_LHS, m_RHS;
};

class CallExprAST : public ExprAST {
public:
    explicit CallExprAST(const std::string &callee, std::vector<std::unique_ptr<ExprAST>> args)
        : m_callee(callee), m_args(std::move(args)) {}

    llvm::Value* codegen() override {
        llvm::Function *callee_f = m_module->getFunction(m_callee);
        if (!callee_f)
            return logErrorV("Unknown function referenced");

        // If argument mismatch error.
        if (callee_f->arg_size() != m_args.size())
            return logErrorV("Incorrect # arguments passed");

        std::vector<llvm::Value *> args_v;
        for (unsigned i = 0, e = m_args.size(); i != e; ++i) {
            args_v.push_back(m_args[i]->codegen());
            if (!args_v.back())
                return nullptr;
        }

        return m_builder->CreateCall(callee_f, args_v, "calltmp");
    }
private:
    llvm::StringRef m_callee;
    std::vector<std::unique_ptr<ExprAST>> m_args;
};

class PrototypeAST {
public:
    PrototypeAST(const std::string& name, std::vector<std::string> args)
        : m_name(name), m_args(std::move(args)) {}

    llvm::Function* codegen() {
        std::vector<llvm::Type*> doubles(m_args.size(), llvm::Type::getDoubleTy(*m_context));
        llvm::FunctionType *ft = llvm::FunctionType::get(llvm::Type::getDoubleTy(*m_context), doubles, false);
        llvm::Function *f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, m_name, m_module.get());

        unsigned idx = 0;
        for (auto &arg : f->args())
            arg.setName(m_args[idx++]);

        return f;
    }
    std::string getName() { return m_name; };
private:
    std::string m_name;
    std::vector<std::string> m_args;
};

class FunctionAST {
public:
    FunctionAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<ExprAST> body)
        : m_prototype(std::move(prototype)), m_body(std::move(body)) {}

    llvm::Function* codegen() {
        llvm::Function* function = m_module->getFunction(m_prototype->getName());
        if (!function)
            function = m_prototype->codegen();

        if (!function)
            return nullptr;

        if (!function->empty())
            return reinterpret_cast<llvm::Function *>(logErrorV("function cannot be redefined"));

        llvm::BasicBlock* bb = llvm::BasicBlock::Create(*m_context, "entry", function);
        m_builder->SetInsertPoint(bb);

        m_named_values.clear();
        for (auto& arg : function->args())
            m_named_values[std::string(arg.getName())] = &arg;

        if (llvm::Value* ret_value = m_body->codegen()) {
            m_builder->CreateRet(ret_value);
            llvm::verifyFunction(*function);
            return function;
        }

        function->eraseFromParent();
        return nullptr;
    }
private:
    std::unique_ptr<PrototypeAST> m_prototype;
    std::unique_ptr<ExprAST> m_body;
};

inline std::unique_ptr<ExprAST> logError(const char *str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

inline std::unique_ptr<PrototypeAST> logErrorP(const char *str) {
    logError(str);
    return nullptr;
}

inline llvm::Value *logErrorV(const char *str) {
    logError(str);
    return nullptr;
}

#endif //PARSER_UTILS_H
