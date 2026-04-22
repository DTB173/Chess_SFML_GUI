module;
#include <vector>
#include <optional>
#include <unordered_map>
#include <string>

import Position;
import Types;
import MoveGen;
import Zobrist;

export module GameState;
namespace GameState {
	using namespace Types;

	constexpr const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    export Square square_from_notation(const char* sq) {
        int file = sq[0] - 'a';
        int rank = sq[1] - '1';
        return Square::from_rf(rank, file);
    }

	export enum class GameResult {
		Ongoing,
		WhiteWins,
		BlackWins,
		Stalemate,
		Draw
	};

	export class GameState {
		Position::Position pos_;
		std::vector<Move> move_history_;
		std::vector<ui64> zobrist_history_;
		GameResult result_ = GameResult::Ongoing;
	public:
		GameState() {
			reset();
		}
		void reset() {
			Zobrist::init();
			pos_.set_fen(startpos);
            zobrist_history_.clear();
			move_history_.clear();
			result_ = GameResult::Ongoing;
		}
        GameResult try_move(int from, int to, char promo_type = 'q') {
			Move m = create_move(from, to, promo_type);
            if (is_move_legal(m)) {
                pos_.make_move(m);
                zobrist_history_.push_back(pos_.get_zobrist_key());
				move_history_.push_back(m);
				result_ = check_win_conditions();
            }

            return result_;
        }

        GameResult check_win_conditions() {
            MoveGen::MoveList moves = MoveGen::generate_all_moves(pos_);
            if (!has_legal_moves(moves)) {
                if (!pos_.is_in_check(pos_.turn())) {
                    return GameResult::Stalemate;
                }
                else {
                    return pos_.turn() == Color::WHITE ? GameResult::BlackWins : GameResult::WhiteWins;
                }
            }
            if (pos_.half_move_count() >= 50) {
                return GameResult::Draw;
            }
            if (is_three_fold()) {
                return GameResult::Draw;
            }
        }

        bool is_three_fold() const {
            if (zobrist_history_.empty()) return false;
            uint64_t current_hash = zobrist_history_.back();
            int count = 0;
            for (auto hash : zobrist_history_) {
                if (hash == current_hash) count++;
            }
            return count >= 3;
        }

        bool has_legal_moves(const MoveGen::MoveList& moves){
            for (auto move : moves) {
                 if (is_move_legal(move)) {
                    return true;
    			 }
            }
            return false;
        }

        Move create_move(int from, int to, char promo_type = 'q') {
            int flags = pos_.get_mailbox()[to].is_none() ? MoveFlags::Quiet : MoveFlags::Capture;

            Piece p = pos_.get_mailbox()[from];

            if (p.type() == PieceType::PAWN) {
                // Double Push
                if (std::abs((from >> 3) - (to >> 3)) == 2) flags = MoveFlags::DoublePush;
                // En Passant
                else if ((from & 7) != (to & 7) && pos_.get_mailbox()[to].is_none()) flags = MoveFlags::EnPassant;
                // Promotion
                if ((to >> 3) == 0 || (to >> 3) == 7) {
                    switch (promo_type) {
                    case 'n': flags = MoveFlags::PromoKnight; break;
                    case 'b': flags = MoveFlags::PromoBishop; break;
                    case 'r': flags = MoveFlags::PromoRook;   break;
                    default:  flags = MoveFlags::PromoQueen;  break;
                    }
                    if (!pos_.get_mailbox()[to].is_none()) flags |= MoveFlags::Capture;
                }
            }
            else if (p.type() == PieceType::KING && std::abs((from & 7) - (to & 7)) == 2) {
                flags = MoveFlags::Castling;
            }

            return { from, to, flags };
        }
		auto& get_mailbox() const {
			return pos_.get_mailbox();
		}

        GameResult get_result() const {
            return result_;
		}

        bool white_to_move() const {
			return pos_.turn() == Color::WHITE;
		}

        bool is_engine_turn(Types::Color user_c)const {
            return pos_.turn() != user_c;
        }

        ui64 get_possible_moves(Square from)const {
            return pos_.get_highlight_bitboard(from);
        }

        ui64 get_attackers_to_square(Square sq, Color defender_side) const {
            return pos_.get_attacks_to(
                sq,
                pos_.get_total_occupancy(),
                defender_side
            );
        }

        ui64 get_king_attackers() const {
            Square kingSq = pos_.get_king_square(pos_.turn());
            return get_attackers_to_square(kingSq, pos_.turn());
        }

        Move get_last_move()const {
            return move_history_.empty() ? Types::NO_MOVE : move_history_.back();
        }

        bool is_move_legal(Move m){
            return Position::is_move_legal(pos_, m);
		}

        const std::vector<Types::Move>& get_move_stack()const {
            return move_history_;
        }
        void undo_move() {
            if (zobrist_history_.empty()) return;
            pos_.undo_move();
            zobrist_history_.pop_back();
			move_history_.pop_back();
        }

        std::string get_uci_position_command() const {
            if (move_history_.empty()) return "position startpos";

            std::string cmd;
			cmd.reserve(25 + 6 * move_history_.size()); // 25 for init comand, 6 for each move (e.g. e2e4, g1f3)
            cmd += "position startpos moves";
            for (const auto& m : move_history_) {
                cmd += " " + Types::move_to_string(m);
            }
            return cmd;
        }
	};

}