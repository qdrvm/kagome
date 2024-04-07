/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wabt/stack_limiter.hpp"

#include "common/visitor.hpp"
#include "log/logger.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/wabt/util.hpp"

namespace kagome::runtime {
  namespace detail {

    template <bool IsConst, typename T>
    struct MaybeConstT;

    template <bool IsConst, typename T>
      requires(!std::is_reference_v<T> && !std::is_pointer_v<T>)
    struct MaybeConstT<IsConst, T> {
      using type = std::conditional_t<IsConst, std::add_const_t<T>, T>;
    };

    template <bool IsConst, typename T>
      requires(std::is_lvalue_reference_v<T>)
    struct MaybeConstT<IsConst, T> {
      using type =
          std::conditional_t<IsConst,
                             std::add_lvalue_reference_t<
                                 std::add_const_t<std::remove_reference_t<T>>>,
                             T>;
    };

    template <bool IsConst, typename T>
      requires(std::is_pointer_v<T>)
    struct MaybeConstT<IsConst, T> {
      using type = std::conditional_t<
          IsConst,
          std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>,
          T>;
    };

    template <bool IsConst, typename T>
    using MaybeConst = typename MaybeConstT<IsConst, T>::type;

    template <bool IsConst>
    struct Frame {
      template <typename T>
      using MaybeConst = typename MaybeConstT<IsConst, T>::type;

      bool is_polymorphic;

      // num of values to be pushed after the exit
      // from the current block.
      uint32_t end_value_num;

      // num of values to be popped upon a branch to
      // this frame.
      uint32_t branch_value_num;

      uint32_t start_height;

      [[nodiscard]] MaybeConst<wabt::ExprList &> getExprList() {
        return visit_in_place(
            top_expr,
            [](MaybeConst<wabt::Func *> func) -> MaybeConst<wabt::ExprList &> {
              return func->exprs;
            },
            [](MaybeConst<wabt::Block *> block)
                -> MaybeConst<wabt::ExprList &> { return block->exprs; },
            [](MaybeConst<typename Frame::Branch &> branch)
                -> MaybeConst<wabt::ExprList &> {
              if (branch.curr_branch == true) {
                return branch.expr->true_.exprs;
              }
              return branch.expr->false_;
            });
      }

      struct Branch {
        MaybeConst<wabt::IfExpr *> expr;
        bool curr_branch = true;
      };
      std::variant<MaybeConst<wabt::Func *>, MaybeConst<wabt::Block *>, Branch>
          top_expr;
      std::conditional_t<IsConst,
                         wabt::ExprList::const_iterator,
                         wabt::ExprList::iterator>
          current_expr;
    };

    using ConstFrame = Frame<true>;
    using MutFrame = Frame<false>;

    // assumed initial stack overhead for any function call
    static constexpr uint32_t ACTIVATION_FRAME_COST = 2;

    template <bool IsConst>
    struct Stack {
      template <typename T>
      using MaybeConst = typename MaybeConstT<IsConst, T>::type;

      using StackFrame = Frame<IsConst>;

      using ExprIt = std::conditional_t<IsConst,
                                        wabt::ExprList::const_iterator,
                                        wabt::ExprList::iterator>;

      explicit Stack(log::Logger logger)
          : height_{ACTIVATION_FRAME_COST},
            frames_{},
            logger_{std::move(logger)} {}

      WabtOutcome<void> unreachable() {
        if (frames_.empty()) {
          return WabtError{"Stack must not be empty"};
        }
        frames_.back().is_polymorphic = true;
        return outcome::success();
      }

      void push_frame(StackFrame frame) {
        SL_DEBUG(logger_,
                 "frame #{}, start height_ {}",
                 frames_.size(),
                 frame.start_height);
        frames_.emplace_back(frame);
      }

      void push_frame(MaybeConst<wabt::Block &> block, bool is_loop) {
        uint32_t end_arity = block.decl.GetNumResults() != 0;
        uint32_t branch_arity = is_loop ? 0 : end_arity;
        push_frame(StackFrame{
            .is_polymorphic = false,
            .end_value_num = end_arity,
            .branch_value_num = branch_arity,
            .start_height = get_height(),
            .top_expr = &block,
            .current_expr = block.exprs.begin(),
        });
      }

      WabtOutcome<void> push_frame(MaybeConst<wabt::IfExpr &> branch,
                                   bool check_frame_boundary) {
        uint32_t end_arity = branch.true_.decl.GetNumResults() != 0;
        uint32_t branch_arity = end_arity;
        auto res = pop_values(1);
        if (check_frame_boundary && !res) {
          return res;
        }
        push_frame(StackFrame{
            .is_polymorphic = false,
            .end_value_num = end_arity,
            .branch_value_num = branch_arity,
            .start_height = get_height(),
            .top_expr = typename StackFrame::Branch{&branch},
            .current_expr = branch.true_.exprs.begin(),
        });
        return outcome::success();
      }

      void push_frame(MaybeConst<wabt::Func &> func) {
        push_frame(StackFrame{
            .is_polymorphic = false,
            .end_value_num = func.GetNumResults(),
            .branch_value_num = func.GetNumResults(),
            .start_height = 0,
            .top_expr = &func,
            .current_expr = func.exprs.begin(),
        });
      }

      WabtOutcome<void> pop_frame() {
        if (frames_.empty()) {
          return WabtError{"Stack is empty"};
        }
        height_ = frames_.back().start_height;
        push_values(frames_.back().end_value_num);

        frames_.pop_back();
        if (frames_.empty()) {
          SL_TRACE(logger_, "pop last frame");
        } else {
          SL_TRACE(logger_,
                   "pop frame, now frame #{}, start height_ {}",
                   frames_.size() - 1,
                   frames_.back().start_height);
        }
        return outcome::success();
      }

      void push_values(uint32_t num) {
        height_ += num;
        SL_TRACE(logger_, "push {}, now height_ {}", num, height_);
      }

      WabtOutcome<void> pop_values(uint32_t num) {
        if (num == 0) {
          return outcome::success();
        }
        if (frames_.empty()) {
          return WabtError{"Stack is empty"};
        }
        SL_TRACE(logger_, "pop {}, now height_ {}", num, height_ - num);
        if (height_ - num < frames_.back().start_height) {
          if (!frames_.back().is_polymorphic) {
            return WabtError{"Popping values not pushed in the current frame"};
          } else {
            return outcome::success();
          }
        }
        if (height_ < num) {
          return WabtError{"Stack underflow"};
        }
        height_ -= num;
        return outcome::success();
      }

      [[nodiscard]] bool empty() const {
        return frames_.empty();
      }

      WabtOutcome<void> advance() {
        bool is_over = false;
        do {
          auto &frame = frames_.back();
          is_over = frame.getExprList().end() == frame.current_expr;
          if (!is_over) {
            ++frame.current_expr;
          }
          is_over = frame.getExprList().end() == frame.current_expr;
          if (is_over) {
            if (std::holds_alternative<typename StackFrame::Branch>(
                    frame.top_expr)) {
              auto &branch =
                  std::get<typename StackFrame::Branch>(frame.top_expr);
              if (branch.curr_branch == true && !branch.expr->false_.empty()) {
                branch.curr_branch = false;
                frame.current_expr = branch.expr->false_.begin();
                break;
              }
            }
            OUTCOME_TRY(pop_frame());
          }
          if (frames_.empty()) {
            return outcome::success();
          }
        } while (is_over);
        return outcome::success();
      }

      [[nodiscard]] uint32_t get_height() const {
        return height_;
      }

      [[nodiscard]] bool is_polymorphic() const {
        return frames_.back().is_polymorphic;
      }

      [[nodiscard]] StackFrame *top_frame() {
        if (frames_.empty()) {
          return nullptr;
        }
        return &frames_.back();
      }

      [[nodiscard]] WabtOutcome<std::reference_wrapper<const StackFrame>>
      get_frame(size_t idx_from_top) const {
        if (frames_.size() <= idx_from_top) {
          return WabtError{"Stack frame underflow"};
        }
        return frames_.at(frames_.size() - idx_from_top - 1);
      }

     private:
      uint32_t height_ = ACTIVATION_FRAME_COST;
      std::vector<StackFrame> frames_;
      log::Logger logger_;
    };

    using ConstStack = Stack<true>;
    using MutStack = Stack<false>;

    WabtOutcome<uint32_t> compute_stack_cost(const log::Logger &logger,
                                             const wabt::Func &func,
                                             const wabt::Module &module) {
      uint32_t locals_num = func.GetNumLocals();

      ConstStack stack{logger};

      stack.push_frame(func);
      if (func.exprs.empty()) {
        return locals_num + ACTIVATION_FRAME_COST;
      }

      uint32_t max_height = 0;

      while (!stack.empty()) {
        auto &top_frame = *stack.top_frame();
        auto &expr = *top_frame.current_expr;
        SL_TRACE(logger, "{}", wabt::GetExprTypeName(expr.type()));
        using wabt::ExprType;

        if (stack.get_height() > max_height && !stack.is_polymorphic()) {
          max_height = stack.get_height();
        }

        bool pushed_frame = false;
        switch (expr.type()) {
          case ExprType::Block: {
            auto *block = dynamic_cast<const wabt::BlockExpr *>(&expr);
            assert(block);
            stack.push_frame(block->block, false);
            pushed_frame = true;
            break;
          }
          case ExprType::If: {
            auto *branch = dynamic_cast<const wabt::IfExpr *>(&expr);
            assert(branch);
            OUTCOME_TRY(stack.push_frame(*branch, true));
            pushed_frame = true;
            break;
          }
          case ExprType::Loop: {
            auto *loop = dynamic_cast<const wabt::LoopExpr *>(&expr);
            assert(loop);
            stack.push_frame(loop->block, true);
            pushed_frame = true;
            break;
          }
          case ExprType::Binary: {
            OUTCOME_TRY(stack.pop_values(2));
            stack.push_values(1);
            break;
          }
          case ExprType::Br: {
            auto *br = dynamic_cast<const wabt::BrExpr *>(&expr);
            assert(br->var.is_index());
            OUTCOME_TRY(frame, stack.get_frame(br->var.index()));
            uint32_t target_arity = frame.get().branch_value_num;
            OUTCOME_TRY(stack.pop_values(target_arity));
            OUTCOME_TRY(stack.unreachable());
            break;
          }
          case ExprType::BrIf: {
            auto *br = dynamic_cast<const wabt::BrIfExpr *>(&expr);
            assert(br->var.is_index());
            OUTCOME_TRY(frame, stack.get_frame(br->var.index()));
            uint32_t target_arity = frame.get().branch_value_num;
            OUTCOME_TRY(stack.pop_values(target_arity));
            OUTCOME_TRY(stack.pop_values(1));
            stack.push_values(target_arity);
            break;
          }
          case ExprType::BrTable: {
            auto *br = dynamic_cast<const wabt::BrTableExpr *>(&expr);
            assert(br->default_target.is_index());
            OUTCOME_TRY(default_frame,
                        stack.get_frame(br->default_target.index()));
            uint32_t target_arity = default_frame.get().branch_value_num;
            for (auto &v : br->targets) {
              assert(v.is_index());
              OUTCOME_TRY(frame, stack.get_frame(v.index()));
              uint32_t arity = frame.get().branch_value_num;
              if (arity != target_arity) {
                return WabtError{
                    "All jump-targets should have equal frame arities"};
              }
            }
            OUTCOME_TRY(stack.pop_values(target_arity));
            OUTCOME_TRY(stack.unreachable());
            break;
          }
          case ExprType::Call: {
            auto *call = dynamic_cast<const wabt::CallExpr *>(&expr);
            assert(call->var.type() == wabt::VarType::Index);
            auto *f = module.GetFunc(call->var);
            OUTCOME_TRY(stack.pop_values(f->GetNumParams()));
            stack.push_values(f->GetNumResults());
            break;
          }
          case ExprType::CallIndirect: {
            auto *call = dynamic_cast<const wabt::CallIndirectExpr *>(&expr);
            OUTCOME_TRY(stack.pop_values(1));
            OUTCOME_TRY(stack.pop_values(call->decl.GetNumParams()));
            stack.push_values(call->decl.GetNumResults());
            break;
          }

          case ExprType::Compare: {
            OUTCOME_TRY(stack.pop_values(2));
            stack.push_values(1);
            break;
          }
          case ExprType::Const:
            stack.push_values(1);
            break;
          case ExprType::Convert: {
            OUTCOME_TRY(stack.pop_values(1));
            stack.push_values(1);
            break;
          }
          case ExprType::Drop: {
            OUTCOME_TRY(stack.pop_values(1));
            break;
          }
          case ExprType::GlobalGet:
            stack.push_values(1);
            break;
          case ExprType::GlobalSet: {
            OUTCOME_TRY(stack.pop_values(1));
            break;
          }
          case ExprType::Load: {
            OUTCOME_TRY(stack.pop_values(1));
            stack.push_values(1);
            break;
          }
          case ExprType::LocalGet:
            stack.push_values(1);
            break;
          case ExprType::LocalSet: {
            OUTCOME_TRY(stack.pop_values(1));
            break;
          }
          case ExprType::LocalTee: {
            OUTCOME_TRY(stack.pop_values(1));
            stack.push_values(1);
            break;
          }
          case ExprType::MemoryGrow: {
            OUTCOME_TRY(stack.pop_values(1));
            stack.push_values(1);
            break;
          }
          case ExprType::MemorySize:
            stack.push_values(1);
            break;
          case ExprType::Nop:
            break;
          case ExprType::RefIsNull: {
            OUTCOME_TRY(stack.pop_values(1));
            stack.push_values(1);
            break;
          }
          case ExprType::RefFunc:
          case ExprType::RefNull:
            stack.push_values(1);
            break;
          case ExprType::Return: {
            OUTCOME_TRY(stack.pop_values(func.GetNumResults()));
            OUTCOME_TRY(stack.unreachable());
            break;
          }
          case ExprType::Select: {
            OUTCOME_TRY(stack.pop_values(2));
            OUTCOME_TRY(stack.pop_values(1));
            stack.push_values(1);
            break;
          }
          case ExprType::LoadZero:
            stack.push_values(1);
            break;
          case ExprType::Store: {
            OUTCOME_TRY(stack.pop_values(2));
            break;
          }
          case ExprType::Ternary: {
            OUTCOME_TRY(stack.pop_values(3));
            stack.push_values(1);
            break;
          }
          case ExprType::Unary: {
            OUTCOME_TRY(stack.pop_values(1));
            stack.push_values(1);
            break;
          }
          case ExprType::Unreachable: {
            OUTCOME_TRY(stack.unreachable());
            break;
          }
          default:
            return WabtError{fmt::format("Unsupported instruction: {}",
                                         wabt::GetExprTypeName(expr.type()))};
        }
        if (!pushed_frame) {
          if (auto expr_opt = stack.advance(); !expr_opt.has_value()) {
            break;
          }
        }
      }
      return max_height + locals_num;
    }

    struct InstrumentCallCtx {
      const wabt::Var &stack_height;
      wabt::Var callee_idx;
      uint32_t callee_stack_cost;
      uint32_t stack_limit;
    };

    wabt::ExprList::iterator instrument_call(const InstrumentCallCtx &ctx,
                                             wabt::ExprList &exprs,
                                             wabt::ExprList::iterator call_it) {
      exprs.insert(call_it,
                   std::make_unique<wabt::GlobalGetExpr>(ctx.stack_height));
      exprs.insert(call_it,
                   std::make_unique<wabt::ConstExpr>(
                       wabt::Const::I32(ctx.callee_stack_cost)));
      exprs.insert(call_it,
                   std::make_unique<wabt::BinaryExpr>(wabt::Opcode::I32Add));
      exprs.insert(call_it,
                   std::make_unique<wabt::GlobalSetExpr>(ctx.stack_height));
      exprs.insert(call_it,
                   std::make_unique<wabt::GlobalGetExpr>(ctx.stack_height));
      exprs.insert(
          call_it,
          std::make_unique<wabt::ConstExpr>(wabt::Const::I32(ctx.stack_limit)));
      exprs.insert(call_it,
                   std::make_unique<wabt::BinaryExpr>(wabt::Opcode::I32GtU));

      auto if_it = exprs.insert(call_it, std::make_unique<wabt::IfExpr>());
      auto &if_expr = static_cast<wabt::IfExpr &>(*if_it);
      wabt::ExprList if_exprs;
      if_exprs.push_back(std::make_unique<wabt::UnreachableExpr>());
      if_expr.true_ = wabt::Block{std::move(if_exprs)};

      auto next_it = exprs.erase(call_it);

      // original call
      auto call_expr = std::make_unique<wabt::CallExpr>(ctx.callee_idx);
      exprs.insert(next_it, std::move(call_expr));

      exprs.insert(next_it,
                   std::make_unique<wabt::GlobalGetExpr>(ctx.stack_height));
      exprs.insert(next_it,
                   std::make_unique<wabt::ConstExpr>(
                       wabt::Const::I32(ctx.callee_stack_cost)));
      exprs.insert(next_it,
                   std::make_unique<wabt::BinaryExpr>(wabt::Opcode::I32Sub));
      exprs.insert(next_it,
                   std::make_unique<wabt::GlobalSetExpr>(ctx.stack_height));
      return next_it;
    }

    WabtOutcome<void> instrument_func(
        wabt::Func &func,
        const wabt::Var &stack_height,
        uint32_t stack_limit,
        const std::unordered_map<wabt::Index, uint32_t> &stack_costs,
        log::Logger logger) {
      if (func.exprs.empty()) {
        return outcome::success();
      }
      MutStack stack{std::move(logger)};

      stack.push_frame(func);

      while (!stack.empty()) {
        bool pushed_frame = false;
        auto &top_frame = *stack.top_frame();
        auto &expr = *top_frame.current_expr;

        using wabt::ExprType;
        switch (expr.type()) {
          case ExprType::Block: {
            auto *block = dynamic_cast<wabt::BlockExpr *>(&expr);
            assert(block);
            stack.push_frame(block->block, false);
            pushed_frame = true;
            break;
          }
          case ExprType::If: {
            auto *branch = dynamic_cast<wabt::IfExpr *>(&expr);
            assert(branch);
            static_cast<void>(stack.push_frame(*branch, false));
            pushed_frame = true;
            break;
          }
          case ExprType::Loop: {
            auto *loop = dynamic_cast<wabt::LoopExpr *>(&expr);
            assert(loop);
            stack.push_frame(loop->block, true);
            pushed_frame = true;
            break;
          }
          case ExprType::Call: {
            auto call = dynamic_cast<wabt::CallExpr *>(&expr);
            assert(call->var.is_index());
            if (auto cost = stack_costs.at(call->var.index()); cost != 0) {
              top_frame.current_expr = instrument_call(
                  InstrumentCallCtx{
                      stack_height,
                      call->var,
                      stack_costs.at(call->var.index()),
                      stack_limit,
                  },
                  top_frame.getExprList(),
                  top_frame.current_expr);
            }
            break;
          }
          case ExprType::CallIndirect:
          case ExprType::CallRef:
          default:
            break;
        }

        if (!pushed_frame) {
          if (auto expr_opt = stack.advance(); !expr_opt.has_value()) {
            return expr_opt.error();
          }
        }
      }

      return outcome::success();
    }

    WabtOutcome<void> generate_thunks(
        const log::Logger &logger,
        wabt::Module &module,
        const wabt::Var &stack_height,
        uint32_t stack_limit,
        const std::unordered_map<wabt::Index, uint32_t> &stack_costs) {
      std::set<wabt::Index> thunked_funcs;
      for (auto *exported : module.exports) {
        if (exported->kind == wabt::ExternalKind::Func) {
          assert(exported->var.is_index());
          thunked_funcs.insert(exported->var.index());
          const wabt::Func *original = module.GetFunc(exported->var);
          SL_TRACE(logger,
                   "Export func: {} ({})",
                   exported->var.index(),
                   original->name);
        }
      }
      for ([[maybe_unused]] auto &elem : module.elem_segments) {
        for (auto &exprs : elem->elem_exprs) {
          assert(exprs.size() == 1);
          auto &expr = exprs.front();
          using wabt::ExprType;
          switch (expr.type()) {
            case ExprType::RefFunc: {
              auto &ref = static_cast<wabt::RefFuncExpr &>(expr);
              assert(ref.var.is_index());
              if (!module.IsImport(wabt::ExternalKind::Func, ref.var)) {
                thunked_funcs.insert(ref.var.index());
                const wabt::Func *original = module.GetFunc(ref.var);
                SL_TRACE(logger,
                         "Element segment func: {} ({})",
                         ref.var.index(),
                         original->name);
              }
              break;
            }
            default:
              return WabtError{
                  fmt::format("Unsupported element expression of type {}",
                              GetExprTypeName(expr.type()))};
          }
        }
      }

      for (auto *start : module.starts) {
        assert(start->is_index());
        thunked_funcs.insert(start->index());
        SL_TRACE(logger, "Start func: {}", start->index());
      }

      std::unordered_map<wabt::Index, wabt::Index> thunked_to_thunk;
      for (auto &thunked : thunked_funcs) {
        const wabt::Func *original = module.funcs.at(thunked);
        assert(original);
        wabt::ExprList thunk;
        for (wabt::Index idx = 0; idx < original->GetNumParams(); idx++) {
          thunk.push_back(std::make_unique<wabt::LocalGetExpr>(
              wabt::Var{idx, wabt::Location{}}));
        }
        wabt::Var v{thunked, {}};
        thunk.push_back(std::make_unique<wabt::CallExpr>(v));
        instrument_call(
            InstrumentCallCtx{
                stack_height, v, stack_costs.at(thunked), stack_limit},
            thunk,
            std::prev(thunk.end()));

        wabt::Func func{""};
        func.exprs = std::move(thunk);
        func.decl = original->decl;

        auto field = std::make_unique<wabt::FuncModuleField>();
        field->func = std::move(func);
        module.AppendField(std::move(field));
        thunked_to_thunk[thunked] = module.funcs.size() - 1;
        SL_TRACE(logger,
                 "Thunk from {} to {} ({})",
                 thunked,
                 module.funcs.size() - 1,
                 original->name);
      }

      for (auto *exported : module.exports) {
        if (exported->kind == wabt::ExternalKind::Func) {
          exported->var.set_index(thunked_to_thunk.at(exported->var.index()));
        }
      }
      for ([[maybe_unused]] auto &elem : module.elem_segments) {
        for (auto &exprs : elem->elem_exprs) {
          assert(exprs.size() == 1);
          auto &expr = exprs.front();
          using wabt::ExprType;
          switch (expr.type()) {
            case ExprType::RefFunc: {
              auto &ref = static_cast<wabt::RefFuncExpr &>(expr);
              if (!module.IsImport(wabt::ExternalKind::Func, ref.var)) {
                ref.var.set_index(thunked_to_thunk.at(ref.var.index()));
              }
              break;
            }
            default:
              return WabtError{
                  fmt::format("Invalid element expression of type {}",
                              GetExprTypeName(expr.type()))};
          }
        }
      }

      for (auto *start : module.starts) {
        start->set_index(thunked_to_thunk.at(start->index()));
      }
      return outcome::success();
    }
  }  // namespace detail

  auto &stackLimiterLog() {
    static auto log = log::createLogger("StackLimiter", "runtime");
    return log;
  }

  WabtOutcome<void> instrumentWithStackLimiter(wabt::Module &module,
                                               size_t stack_limit) {
    auto logger = stackLimiterLog();
    KAGOME_PROFILE_START_L(logger, count_costs);
    std::unordered_map<wabt::Index, uint32_t> func_costs;
    for (size_t i = 0; i < module.num_func_imports; i++) {
      func_costs[i] = 0;
    }
    for (size_t i = module.num_func_imports; i < module.funcs.size(); i++) {
      auto &func = module.funcs[i];
      SL_TRACE(logger, "count cost {}", func->name);
      OUTCOME_TRY(cost, detail::compute_stack_cost(logger, *func, module));
      func_costs[i] = cost;
      SL_TRACE(logger, "cost {} = {}", func->name, cost);
    }
    KAGOME_PROFILE_END_L(logger, count_costs);

    wabt::Global stack_height_global{""};
    stack_height_global.type = wabt::Type::I32;
    stack_height_global.init_expr.push_back(
        std::make_unique<wabt::ConstExpr>(wabt::Const::I32(0)));
    stack_height_global.mutable_ = true;
    auto stack_height_field = std::make_unique<wabt::GlobalModuleField>();
    stack_height_field->global = std::move(stack_height_global);
    module.AppendField(std::move(stack_height_field));
    wabt::Index stack_height_index = module.globals.size() - 1;
    wabt::Var stack_height_var{stack_height_index, wabt::Location{}};

    KAGOME_PROFILE_START_L(logger, instrument_wasm);
    for (size_t i = 0; i < module.funcs.size(); i++) {
      auto &func = module.funcs[i];
      OUTCOME_TRY(detail::instrument_func(
          *func, stack_height_var, stack_limit, func_costs, logger));
      SL_TRACE(logger, "[{}/{}] {}", i, module.funcs.size(), func->name);
    }

    OUTCOME_TRY(detail::generate_thunks(
        logger, module, stack_height_var, stack_limit, func_costs));

    KAGOME_PROFILE_END_L(logger, instrument_wasm);

    OUTCOME_TRY(wabtValidate(module));
    return outcome::success();
  }

  WabtOutcome<common::Buffer> instrumentWithStackLimiter(
      common::BufferView uncompressed_wasm, size_t stack_limit) {
    auto logger = stackLimiterLog();
    KAGOME_PROFILE_START_L(logger, read_ir);
    OUTCOME_TRY(module, wabtDecode(uncompressed_wasm));
    KAGOME_PROFILE_END_L(logger, read_ir);

    OUTCOME_TRY(instrumentWithStackLimiter(module, stack_limit));

    KAGOME_PROFILE_START_L(logger, serialize_wasm);
    return wabtEncode(module);
  }
}  // namespace kagome::runtime
