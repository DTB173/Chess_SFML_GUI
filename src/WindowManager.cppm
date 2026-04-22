module;
#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>

export module WindowManager;
import Types;
import Position;
import Button;
import GameState;
import Bitboards;
import EngineInterface;
import PlayerCard;
import MovePanel;

namespace Settings {
	constexpr int WINDOW_WIDTH = 1280;
	constexpr int WINDOW_HEIGHT = 720;
    constexpr double ASPECT = static_cast<double>(WINDOW_WIDTH) / WINDOW_HEIGHT;

    constexpr int MIN_WIDTH = 640;
    constexpr int MIN_HEIGHT = 480;

	constexpr int BOARD_SIZE = 8;

    constexpr int TILE_SIZE = (WINDOW_HEIGHT * 0.9) / BOARD_SIZE;

    constexpr int BOARD_OFFSET_X = WINDOW_WIDTH/4;
    constexpr int BOARD_OFFSET_Y = (WINDOW_HEIGHT - (BOARD_SIZE * TILE_SIZE)) / 2;

    constexpr int UI_X = BOARD_OFFSET_X + (BOARD_SIZE * TILE_SIZE) + 40;

    // Tiles
    constexpr sf::Color WHITE_TILE_COLOR(240, 217, 181);
    constexpr sf::Color BLACK_TILE_COLOR(181, 136, 99);
	constexpr sf::Color HIGHLIGHT_COLOR(170, 160, 0, 150);
    constexpr sf::Color LAST_MOVE_COLOR(247, 247, 105, 120);
    constexpr sf::Color ATTACKER_COLOR(235, 65, 60, 160);

    // UI
    constexpr sf::Color BACKGROUND_COLOR(38, 36, 33);
    constexpr sf::Color BUTTON_IDLE(60, 58, 55);
    constexpr sf::Color BUTTON_HOVER(80, 78, 75);

    // Eval
    constexpr sf::Color EVAL_BAR_BLACK(25, 25, 25);
    constexpr sf::Color EVAL_BAR_WHITE(255, 255, 255);
}

namespace WindowManager {
	enum class Screens {
		MainMenu,
		Game,
		PauseMenu,
        Promotion,
        GameEnd
	};

    enum class Mode{
        PVP,
        AI
    };

    export class GameWindow {
		GameState::GameState game_state_;
		std::unique_ptr<sf::RenderWindow> window_ = nullptr;
		Screens current_screen_ = Screens::MainMenu;
		Mode current_mode_ = Mode::PVP;
		std::array<std::array<sf::Texture,2>,6> pieces_texture_;
        sf::Font font_;

        std::unique_ptr<Button::TextButton> undo_button_;
        std::unique_ptr<Button::TextButton> resign_button_;

        std::unique_ptr<Button::TextButton> continue_button_;
        std::unique_ptr<Button::TextButton> quit_button_;
        std::unique_ptr<Button::TextButton> new_game_button_;
        std::unique_ptr<Button::TextButton> mode_button_;

        std::unique_ptr<Cards::PlayerCard> white_card_;
        std::unique_ptr<Cards::PlayerCard> black_card_;
        std::unique_ptr<Moves::MoveListUI> move_list_;

		std::array<std::array<std::unique_ptr<Button::ImageButton>, 2>,4> promotion_buttons_;
        std::array<std::unique_ptr<Button::TextButton>, 4> think_time_buttons_;

		std::optional<int> selected_square_ = std::nullopt;
		std::optional<int> clicked_square_ = std::nullopt;

        std::unique_ptr<sf::Text> engine_label_;
        std::unique_ptr<sf::Text> engine_info_;

        Types::Move last_engine_move_ = Types::NO_MOVE;
		int engine_think_time_ms_ = 5000;
		bool is_engine_thinking_ = false;
        
        sf::SoundBuffer move_sound_buffer_;
        sf::SoundBuffer capture_sound_buffer_;

        std::unique_ptr<sf::Sound> move_sound_;
        std::unique_ptr<sf::Sound> capture_sound_;

		Types::Color user_side_ = Types::Color::WHITE;
		EngineInterface::EngineInterface engine_;
	public:
        GameWindow():engine_(L"chess_engine.exe") {
            window_ = std::make_unique<sf::RenderWindow>(sf::VideoMode({ Settings::WINDOW_WIDTH, Settings::WINDOW_HEIGHT }), "Chess");
            window_->setFramerateLimit(60);

            load_resources();
            setup_buttons();
            setup_labels();

            white_card_ = std::make_unique<Cards::PlayerCard>(180, 40, Settings::BUTTON_IDLE, Settings::BUTTON_HOVER, font_, "");
            black_card_ = std::make_unique<Cards::PlayerCard>(180, 40, Settings::BUTTON_IDLE, Settings::BUTTON_HOVER, font_, "");

            float list_size_y = Settings::WINDOW_HEIGHT - 2 * (Settings::BOARD_OFFSET_Y + 40 + 10);
            move_list_ = std::make_unique<Moves::MoveListUI>(game_state_.get_move_stack(), font_, sf::Vector2f{180.0f, list_size_y});
        }

		GameWindow(const GameWindow&) = delete;
		
		void game_loop() {
			while (window_->isOpen()) {
				handle_events();
				update();
				render();
			}
		}
	private:
        void load_resources() {
            static const std::array<std::string, 2> teams = { "white","black" };
            static const std::array<std::string, 6> types = { "pawn","rook","knight","bishop","queen","king" };
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < 6; j++) {
                    if (!pieces_texture_[j][i].loadFromFile("assets/Pieces/" + types[j] + "_" + teams[i] + ".png")) {
						std::cerr << "Failed to load texture for " << teams[i] << " " << types[j] << '\n';
                    }
                    pieces_texture_[j][i].setSmooth(true);
                }
            }

            if (move_sound_buffer_.loadFromFile("assets/Sounds/move-self.mp3")) {
                move_sound_ = std::make_unique<sf::Sound>(move_sound_buffer_);
            }
            else {
                std::cerr << "Error loading: move-self.mp3\n";
            }
            if (capture_sound_buffer_.loadFromFile("assets/Sounds/capture.mp3")) {
                capture_sound_ = std::make_unique<sf::Sound>(capture_sound_buffer_);
            }
            else {
                std::cerr << "Error loading: capture.mp3\n";
            }


            if (!font_.openFromFile("assets/Fonts/CooperHewitt.otf"))
                std::cerr << "Failed to load font" << '\n';
		}

        void setup_buttons(){
            undo_button_ = std::make_unique<Button::TextButton>(font_, "Undo", sf::Vector2f(140.f, 40.f));
            resign_button_ = std::make_unique<Button::TextButton>(font_, "Resign", sf::Vector2f(140.f, 40.f));
            continue_button_ = std::make_unique<Button::TextButton>(font_, "Continue", sf::Vector2f(140.f, 40.f));
            quit_button_ = std::make_unique<Button::TextButton>(font_, "Quit", sf::Vector2f(140.f, 40.f));
            new_game_button_ = std::make_unique<Button::TextButton>(font_, "New Game", sf::Vector2f(140.f, 40.f));
            mode_button_ = std::make_unique<Button::TextButton>(font_, "Mode: PvP", sf::Vector2f(140.f, 40.f));

            int padding = 20;
            const char piece_chars[] = { 'p', 'r', 'n', 'b', 'q', 'k' };
            int start_x = (Settings::WINDOW_WIDTH - (4 * Settings::TILE_SIZE + 3 * padding)) / 2;
            int start_y = (Settings::WINDOW_HEIGHT - Settings::TILE_SIZE) / 2;

            for (int color = 0; color < 2; ++color) {
                int start_x = (Settings::WINDOW_WIDTH - (4 * Settings::TILE_SIZE + 3 * padding)) / 2;
                for (int type = 0; type < 4; ++type) {
                    promotion_buttons_[type][color] = std::make_unique<Button::ImageButton>(pieces_texture_[type + 1][color], sf::Vector2f(Settings::TILE_SIZE, Settings::TILE_SIZE));
                    promotion_buttons_[type][color]->set_on_click([this, type, color, piece_chars]() {
                        char choice = piece_chars[type + 1];

                        GameState::GameResult res = game_state_.try_move(*selected_square_, *clicked_square_, choice);
                        current_screen_ = res != GameState::GameResult::Ongoing ? Screens::GameEnd : Screens::Game;

                        selected_square_ = std::nullopt;
                        clicked_square_ = std::nullopt; }
                    );
                    promotion_buttons_[type][color]->set_position(sf::Vector2f(start_x, start_y));
                    start_x += Settings::TILE_SIZE + padding;
                }
            }

            undo_button_->set_on_click([this]() {
                game_state_.undo_move();
                std::cout << "Undo clicked\n";
                if (current_mode_ != Mode::AI) return;

                is_engine_thinking_ = false;
                engine_.stop();
                last_engine_move_ = Types::NO_MOVE;
                if (game_state_.is_engine_turn(user_side_)) {
                    is_engine_thinking_ = true;
                    engine_.start_thinking(game_state_.get_uci_position_command(), engine_think_time_ms_);
                }
                });

            resign_button_->set_on_click([this]() {
                current_screen_ = Screens::MainMenu;
                std::cout << "Resign clicked\n";

                if (current_mode_ != Mode::AI) return;
                engine_.stop();
                is_engine_thinking_ = false;
                last_engine_move_ = Types::NO_MOVE;
                });

            continue_button_->set_on_click([this]() {
                current_screen_ = Screens::Game;
                std::cout << "Continue clicked\n";
                });

            quit_button_->set_on_click([this]() {
                std::cout << "Quit clicked\n";
                if (current_screen_ == Screens::PauseMenu) current_screen_ = Screens::MainMenu;
                else  window_->close();
                });

            new_game_button_->set_on_click([this]() {
                current_screen_ = Screens::Game;
                game_state_.reset();
                engine_.stop();
                engine_.new_game();
                selected_square_ = std::nullopt;
                user_side_ = std::rand() % 2 == 0 ? Types::Color::WHITE : Types::Color::BLACK;
                if (user_side_ == Types::Color::BLACK && current_mode_ == Mode::AI) {
                    std::cout << "Player side: " << (user_side_ == Types::Color::WHITE ? "White" : "Black") << '\n';
                    is_engine_thinking_ = true;
                    engine_.start_thinking(game_state_.get_uci_position_command(), engine_think_time_ms_);
                }

                if (current_mode_ == Mode::AI) {
                    black_card_->set_position(15, Settings::BOARD_OFFSET_Y);

                    auto size = white_card_->get_size();
                    white_card_->set_position(15, Settings::WINDOW_HEIGHT - Settings::BOARD_OFFSET_Y - size.y);

                    float list_y = Settings::BOARD_OFFSET_Y + black_card_->get_size().y + 10.0;
                    move_list_->set_position(15, list_y);
                }
                else {
                    float x_limit = Settings::BOARD_OFFSET_X - 30.0f;
                    float center_x = x_limit / 2.0f;
                    float card_x = center_x - (black_card_->get_size().x / 2.0f);

                    black_card_->set_position(card_x, Settings::BOARD_OFFSET_Y);

                    auto size = white_card_->get_size();
                    white_card_->set_position(card_x, Settings::WINDOW_HEIGHT - Settings::BOARD_OFFSET_Y - size.y);

                    float list_y = Settings::BOARD_OFFSET_Y + black_card_->get_size().y + 10.0;
                    move_list_->set_position(card_x, list_y);
                }
                std::cout << "Player side: " << (user_side_ == Types::Color::WHITE ? "White" : "Black") << '\n';
                std::cout << "New Game clicked\n";
                });

            mode_button_->set_on_click([this]() {
                std::array<std::string, 2> modes = { "Mode: PvP", "Mode: PvAI" };
                current_mode_ = (current_mode_ == Mode::PVP) ? Mode::AI : Mode::PVP;
                mode_button_->set_label(modes[static_cast<int>(current_mode_)]);
                user_side_ = std::rand() % 2 == 0 ? Types::Color::WHITE : Types::Color::BLACK;
                std::cout << "Mode switched to: " << modes[static_cast<int>(current_mode_)] << '\n';
                });

            float ui_x = static_cast<float>(Settings::BOARD_OFFSET_X + (Settings::BOARD_SIZE * Settings::TILE_SIZE) + 30);
            undo_button_->set_position({ ui_x, Settings::WINDOW_HEIGHT * 0.75f });
            resign_button_->set_position({ ui_x, Settings::WINDOW_HEIGHT * 0.85f });

            std::array<int, 4> think_times = { 5, 10, 15, 30 };
            float b_start_y = Settings::WINDOW_HEIGHT * 0.3f;
            float spacing = 50.f;

            for (int i = 0; i < think_time_buttons_.size(); ++i) {
                int time = think_times[i];
                think_time_buttons_[i] = std::make_unique<Button::TextButton>(
                    font_, std::to_string(time) + "s", sf::Vector2f(140.f, 40.f)
                );

                think_time_buttons_[i]->set_position({ ui_x, b_start_y + (i * spacing) });

                think_time_buttons_[i]->set_on_click([this, time]() {
                    this->engine_think_time_ms_ = time * 1000;
                    engine_label_->setString("Engine Time: " + std::to_string(time) + "s");
                    std::cout << "Engine time set to: " << time << "s\n";
                    });
            }
        }

        void setup_labels() {
            engine_label_ = std::make_unique<sf::Text>(font_, "Engine Time: " + std::to_string(engine_think_time_ms_ / 1000) + "s", 18);

            std::string engine_data = std::format(
                "{:<8} {}\n"
                "{:<9} {}\n"
                "{:<10} {:.2f}M\n",
                "Status:", is_engine_thinking_ ? "Thinking" : "Idle",
                "Depth:", 0,
                "NPS:", 0.0f
            );

            engine_info_ = std::make_unique<sf::Text>(font_, engine_data, 18);

            engine_label_->setFillColor(sf::Color(200, 200, 200));
            engine_info_->setFillColor(sf::Color(200, 200, 200));


            float ui_x = static_cast<float>(Settings::BOARD_OFFSET_X + (Settings::BOARD_SIZE * Settings::TILE_SIZE) + 30);
            float label_y = (Settings::WINDOW_HEIGHT * 0.3f) - 30.f;
            engine_label_->setPosition({ ui_x, label_y });
            engine_info_->setPosition({ ui_x, Settings::BOARD_OFFSET_Y });
        }

        void handle_resize(const std::optional<sf::Event>& event) {
            auto* ev = event->getIf<sf::Event::Resized>();
            unsigned width = ev->size.x;
            unsigned height = ev->size.y;

            // Enforce minimum size
            if (width < Settings::MIN_WIDTH) width = Settings::MIN_WIDTH;
            if (height < Settings::MIN_HEIGHT) height = Settings::MIN_HEIGHT;

            // Enforce aspect ratio
            float currentAspect = static_cast<float>(width) / height;

            if (currentAspect > Settings::ASPECT)
            {
                width = static_cast<unsigned>(height * Settings::ASPECT);
            }
            else
            {
                height = static_cast<unsigned>(width / Settings::ASPECT);
            }

            window_->setSize({ width, height });
        }

        void handle_events() {
            while (const std::optional event = window_->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window_->close();
                }

                if (event->is<sf::Event::Resized>()) {
                    handle_resize(event);
                }
                if (current_screen_ == Screens::Game) {
                    handle_game_input(event);
                }
                else if(current_screen_ == Screens::Promotion) {
                    handle_promotion_input(event);
				}
                else if (current_screen_ == Screens::GameEnd) {
                    handle_menu_input(event);
                }
                else {
					handle_menu_input(event);
                }
            }
        }

        void handle_menu_input(const std::optional<sf::Event>& event) {
            if (!event) return;

            sf::Vector2f mouse_pos = window_->mapPixelToCoords(sf::Mouse::getPosition(*window_));

            if (event->is<sf::Event::MouseMoved>()) {
                if (current_screen_ == Screens::MainMenu || current_screen_ == Screens::GameEnd) {
                    new_game_button_->update_hover(mouse_pos);
					mode_button_->update_hover(mouse_pos);
                }
                else if (current_screen_ == Screens::PauseMenu) {
                    continue_button_->update_hover(mouse_pos);
                }
                quit_button_->update_hover(mouse_pos);
            }

            if (auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    if (current_screen_ == Screens::MainMenu || current_screen_ == Screens::GameEnd) {
                        new_game_button_->check_click(mouse_pos);
						mode_button_->check_click(mouse_pos);
                    }
                    else if (current_screen_ == Screens::PauseMenu) {
                        continue_button_->check_click(mouse_pos);
                    }
                    quit_button_->check_click(mouse_pos);
                }
            }

            if (auto* kb = event->getIf<sf::Event::KeyPressed>()) {
                if (kb->code == sf::Keyboard::Key::Escape && current_screen_ == Screens::PauseMenu) current_screen_ = Screens::Game;
            }
        }

        void handle_game_input(const std::optional<sf::Event>& event) {
            if (!event) return;

            if (auto* kb = event->getIf<sf::Event::KeyPressed>()) {
                if (kb->code == sf::Keyboard::Key::Escape) {
                    current_screen_ = Screens::PauseMenu;
                    return;
                }
            }

            sf::Vector2f mouse_pos = window_->mapPixelToCoords(sf::Mouse::getPosition(*window_));

            if (event->is<sf::Event::MouseMoved>()) {
                resign_button_->update_hover(mouse_pos);
                undo_button_->update_hover(mouse_pos);

                for(int button_idx = 0; button_idx < think_time_buttons_.size(); ++button_idx) {
                    think_time_buttons_[button_idx]->update_hover(mouse_pos);
				}
            }

            if (auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    resign_button_->check_click(mouse_pos);
                    undo_button_->check_click(mouse_pos);

                    for (int button_idx = 0; button_idx < think_time_buttons_.size(); ++button_idx) {
                        think_time_buttons_[button_idx]->check_click(mouse_pos);
                    }

                    auto clicked_sq = get_square_from_mouse(mouse_pos);
                    if (!clicked_sq) return;

                    const auto piece_at_click = game_state_.get_mailbox()[*clicked_sq];

                    if (!selected_square_) {
                        if (!piece_at_click.is_none()) {
                            selected_square_ = clicked_sq;
                        }
                    }
                    else {
                        if (clicked_sq == selected_square_) {
                            selected_square_ = std::nullopt;
                            return;
                        }

                        bool is_human_turn = true;
                        if (current_mode_ == Mode::AI) {
                            bool turn_is_white = game_state_.white_to_move();
                            is_human_turn = (turn_is_white && user_side_ == Types::Color::WHITE) ||
                                (!turn_is_white && user_side_ == Types::Color::BLACK);
                        }

                        const auto selected_piece = game_state_.get_mailbox()[*selected_square_];
                        bool owns_piece = (selected_piece.is_white() && user_side_ == Types::Color::WHITE) ||
                            (!selected_piece.is_white() && user_side_ == Types::Color::BLACK);

                        if (current_mode_ == Mode::PVP) {
                            owns_piece = (selected_piece.is_white() == game_state_.white_to_move());
                        }

                        if (is_human_turn && owns_piece) {
                            Types::Move m = game_state_.create_move(*selected_square_, *clicked_sq);

                            if (game_state_.is_move_legal(m)) {
                                if (m.is_promo()) {
                                    clicked_square_ = clicked_sq;
                                    current_screen_ = Screens::Promotion;
                                }
                                else {
                                    GameState::GameResult res = game_state_.try_move(*selected_square_, *clicked_sq);
                                    if (res == GameState::GameResult::Ongoing) {
                                        auto m = game_state_.get_last_move();
                                        if (m.is_capture()) {
                                            capture_sound_->play();
                                        }
                                        else {
                                            move_sound_->play();
                                        }
                                        if (current_mode_ == Mode::AI) {
                                            is_engine_thinking_ = true;
                                            engine_.start_thinking(game_state_.get_uci_position_command(), engine_think_time_ms_);
                                        }
                                    }
                                    else {
										current_screen_ = Screens::GameEnd;
                                    }
                                    selected_square_ = std::nullopt;
                                }
                                return;
                            }
                        }

                        if (!piece_at_click.is_none()) {
                            selected_square_ = clicked_sq;
                        }
                        else {
                            selected_square_ = std::nullopt;
                        }
                    }
                }
            }
        }

        void handle_promotion_input(const std::optional<sf::Event>& event) {
            if (!event) return;
            sf::Vector2f mouse_pos = window_->mapPixelToCoords(sf::Mouse::getPosition(*window_));
            int color_idx = game_state_.white_to_move() ? 0 : 1;

            if (event->is<sf::Event::MouseMoved>()) {
                for (int type = 0; type < 4; ++type) {
                    promotion_buttons_[type][color_idx]->update_hover(mouse_pos);
                }
            }

            if (auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    for (int type = 0; type < 4; ++type) {
                        if (promotion_buttons_[type][color_idx]->check_click(mouse_pos)) {
                            return;
                        }
                    }
                }
            }
        }

        std::optional<int> get_square_from_mouse(const sf::Vector2f mouse_pos) {
			int file = static_cast<int>((mouse_pos.x - Settings::BOARD_OFFSET_X) / Settings::TILE_SIZE);
			int rank = 7 - static_cast<int>((mouse_pos.y - Settings::BOARD_OFFSET_Y) / Settings::TILE_SIZE);

			return (file >= 0 && file < 8 && rank >= 0 && rank < 8) ? std::optional<int>(rank * 8 + file) : std::nullopt;
        }


        void update() {
            if (current_mode_ == Mode::AI) {
                white_card_->set_text(user_side_ == Types::Color::WHITE ? "YOU (W)" : "AI (W)");
                black_card_->set_text(user_side_ == Types::Color::WHITE ? "AI (B)"  : "YOU (B)");
            }
            else {
                white_card_->set_text(user_side_ == Types::Color::WHITE ? "Player 1 (W)" : "Player 1 (W)");
                black_card_->set_text(user_side_ == Types::Color::WHITE ? "Player 2 (B)" : "Player 2 (B)");
            }

            if (game_state_.white_to_move()) {
                white_card_->set_active();
                black_card_->set_inactive();
            }
            else {
                white_card_->set_inactive();
                black_card_->set_active();
            }

            if (!is_engine_thinking_) return;
            std::string move_str = engine_.consume_best_move();
            if (!move_str.empty()) {
                is_engine_thinking_ = false;

                int from = GameState::square_from_notation(move_str.substr(0, 2).c_str());
                int to = GameState::square_from_notation(move_str.substr(2, 2).c_str());

                GameState::GameResult res;
                if (move_str.length() > 4) {
                    res = game_state_.try_move(from, to, move_str[4]);                   
                }
                else {
                    res = game_state_.try_move(from, to);
                }

                if (res != GameState::GameResult::Ongoing) {
                    current_screen_ = Screens::GameEnd;
                }

                auto m = game_state_.get_last_move();
                if (m.is_capture()) {
                    capture_sound_->play();
                }
                else {
                    move_sound_->play();
                }

                std::cout << "Engine moved: " << move_str << std::endl;
            }
            auto info = engine_.get_engine_info();
            std::string engine_data = std::format(
                "{:<8} {}\n"
                "{:<9} {}\n"
                "{:<10} {:.2f}M\n",
                "Status:", is_engine_thinking_ ? "Thinking" : "Idle",
                "Depth:", info.depth,
                "NPS:", info.nps
            );
            engine_info_->setString(engine_data);
        }

        void render() {
            window_->clear(sf::Color(30, 30, 30));

            if (current_screen_ == Screens::Game) {
                draw_game();
            }
            else if(current_screen_ == Screens::Promotion) {
				draw_board_elements();
                draw_overlay("Pick piece");

                int color_idx = game_state_.white_to_move() ? 0 : 1;

                for (int type = 0; type < 4; ++type) {
                    promotion_buttons_[type][color_idx]->draw(*window_);
                }
			}
            else if (current_screen_ == Screens::GameEnd) {
                draw_board_elements();
				static const std::array<std::string, 5> results = { "Ongoing", "White Wins!", "Black Wins!","Stalemate", "Draw"};
				draw_overlay(results[static_cast<int>(game_state_.get_result())]);
                draw_menu_buttons();
            }
            else {    
                draw_menu();
            }

            window_->display();
        }

        void draw_board() {
            using Settings::TILE_SIZE;
            sf::RectangleShape white_square({ TILE_SIZE, TILE_SIZE });
            sf::RectangleShape black_square({ TILE_SIZE, TILE_SIZE });
			sf::RectangleShape selection_highlight({ TILE_SIZE, TILE_SIZE });
            sf::CircleShape attacker_circle(TILE_SIZE * 0.5f);
            attacker_circle.setOrigin({ attacker_circle.getRadius(), attacker_circle.getRadius() });

            sf::RectangleShape last_move_sq({ TILE_SIZE, TILE_SIZE });

            white_square.setFillColor(Settings::WHITE_TILE_COLOR);
            black_square.setFillColor(Settings::BLACK_TILE_COLOR);
            selection_highlight.setFillColor(Settings::HIGHLIGHT_COLOR);
            attacker_circle.setFillColor(Settings::ATTACKER_COLOR);

            last_move_sq.setFillColor(Settings::LAST_MOVE_COLOR);

            std::uint64_t attackers = game_state_.get_king_attackers();

            for(int square = 0; square < 64; ++square) {
                int rank = square / 8;
                int col = square % 8;
                int visual_row = 7 - rank;

                sf::Vector2f tile_pos = sf::Vector2f(Settings::BOARD_OFFSET_X + col * TILE_SIZE,
                                                     Settings::BOARD_OFFSET_Y + visual_row * TILE_SIZE);

                if ((rank + col) % 2 != 0) {
                    white_square.setPosition(tile_pos);
                    window_->draw(white_square);
                }
                else {
                    black_square.setPosition(tile_pos);
                    window_->draw(black_square);
                }

                if (selected_square_ && square == selected_square_.value()) {
                    selection_highlight.setPosition(tile_pos);
                    window_->draw(selection_highlight);
                    continue;
                }

                if (auto move = game_state_.get_last_move(); move != Types::NO_MOVE) {
                    int from = move.from();
                    int to = move.to();
                    if (square == from || square == to) {
                        last_move_sq.setPosition(tile_pos);
                        window_->draw(last_move_sq);
                    }
                }

                if (attackers & 1ULL << square) {
                    attacker_circle.setPosition({ tile_pos.x + TILE_SIZE / 2.f,
                                                  tile_pos.y + TILE_SIZE / 2.f });
                    window_->draw(attacker_circle);
                }
			}
            if(current_mode_ == Mode::AI)  draw_evaluation_bar();
            draw_coordinates();
        }



        void draw_evaluation_bar() {
            const float width = 32.f;
            const float height = 8 * Settings::TILE_SIZE;
            const float x = Settings::BOARD_OFFSET_X - Settings::TILE_SIZE - 10.f;
            const float y = Settings::BOARD_OFFSET_Y;

            int raw_eval = engine_.get_eval();
            raw_eval = user_side_ == Types::Color::WHITE ? -raw_eval : raw_eval;
            bool is_mate = engine_.is_mate_score();

            float white_view_pct;

            if (is_mate) {
                int mate_pl = engine_.get_mate_in_plies();
                white_view_pct = (mate_pl > 0) ? 1.0f : 0.0f;
            }
            else {
                constexpr float K = 0.004f;
                float clamped = std::clamp(static_cast<float>(raw_eval) / 100.0f, -10.f, 10.f);
                white_view_pct = 0.5f + 0.5f * std::tanh(K * clamped * 100.0f);
            }

            sf::RectangleShape bg({ width + 4.f, height + 4.f });
            bg.setFillColor(sf::Color(28, 26, 24));
            bg.setOutlineColor(sf::Color(80, 75, 70));
            bg.setOutlineThickness(1.f);
            bg.setPosition({ x - 2.f, y - 2.f });
            window_->draw(bg);

            sf::RectangleShape bar({ width, height });
            bar.setFillColor(sf::Color(38, 36, 33));
            bar.setPosition({ x, y });
            window_->draw(bar);

            float white_h = height * white_view_pct;
            sf::RectangleShape white_part({ width, white_h });
            white_part.setFillColor(sf::Color(245, 245, 240));
            white_part.setPosition({ x, y + height - white_h });
            window_->draw(white_part);

            if (!is_mate) {
                sf::VertexArray gradient(sf::PrimitiveType::TriangleStrip, 4);

                gradient[0].position = { x, y + height - white_h };
                gradient[0].color = sf::Color(255, 255, 255, 180);

                gradient[1].position = { x + width, y + height - white_h };
                gradient[1].color = sf::Color(255, 255, 255, 180);

                gradient[2].position = { x, y + height };
                gradient[2].color = sf::Color(220, 220, 220, 255);

                gradient[3].position = { x + width, y + height };
                gradient[3].color = sf::Color(220, 220, 220, 255);

                window_->draw(gradient);
            }

            sf::RectangleShape midline({ width + 6.f, 2.f });
            midline.setFillColor(sf::Color(160, 140, 100, 200));
            midline.setPosition({ x - 3.f, y + height / 2.f - 1.f });
            window_->draw(midline);

            draw_eval_text(x, y, height, raw_eval, is_mate);
        }

        void draw_eval_text(float x, float y, float height, int raw_eval, bool is_mate) {
            std::string txt;
            if (is_mate) {
                int plies = engine_.get_mate_in_plies();
                int moves = (std::abs(plies) + 1) / 2;
                txt = "M" + std::to_string(moves);
            }
            else {
                float pawns = raw_eval / 100.0f;
                std::stringstream ss;
                ss << (pawns >= 0 ? "+" : "") << std::fixed << std::setprecision(1) << std::abs(pawns);
                txt = ss.str();
            }

            sf::Text text(font_, txt, 14);

            if (is_mate) {
                text.setFillColor(sf::Color(255, 215, 0));
            }
            else {
                text.setFillColor(raw_eval >= 0 ? sf::Color(20, 20, 20) : sf::Color(240, 240, 240));
            }

            sf::FloatRect bounds = text.getLocalBounds();

            float text_x = x + (32.f - bounds.size.x) / 2.f - bounds.position.x;

            float text_y;
            if (is_mate || std::abs(raw_eval) < 100) {
                text_y = y + (height / 2.f) - (bounds.size.y / 2.f) - bounds.position.y;
            }
            else if (raw_eval > 0) {
                text_y = y + height - bounds.size.y - 10.f;
            }
            else {
                text_y = y + 10.f;
            }

            text.setPosition({ text_x, text_y });
            window_->draw(text);
        }

        void draw_coordinates() {
            sf::Text text(font_, "", 14);
            text.setFillColor(sf::Color(160, 160, 160));

            for (int i = 0; i < 8; ++i) {
                text.setString(static_cast<char>('A' + i));
                float x_file = Settings::BOARD_OFFSET_X + (i * Settings::TILE_SIZE) + (Settings::TILE_SIZE / 2.0f);
                float y_file = Settings::BOARD_OFFSET_Y + (8 * Settings::TILE_SIZE) + 5;

                sf::FloatRect bounds = text.getLocalBounds();
                text.setOrigin(bounds.getCenter());
                text.setPosition({ x_file, y_file + 10 });
                window_->draw(text);

                text.setString(std::to_string(8 - i));
                float x_rank = Settings::BOARD_OFFSET_X - 15;
                float y_rank = Settings::BOARD_OFFSET_Y + (i * Settings::TILE_SIZE) + (Settings::TILE_SIZE / 2.0f);

                bounds = text.getLocalBounds();
                text.setOrigin(bounds.getCenter());
                text.setPosition({ x_rank, y_rank });
                window_->draw(text);
            }
        }

        void draw_pieces() {
            using namespace Types;

            auto& mailbox = game_state_.get_mailbox();

            for (int square = 0; square < 64; ++square) {
                auto piece = mailbox[square];

                if (piece.is_none()) continue;

                int type_idx = static_cast<int>(piece.type()) - 1;
                int color_idx = static_cast<int>(piece.color()) - 1;

                sf::Sprite sprite(pieces_texture_[type_idx][color_idx]);

                int file = square % 8;
                int rank = square / 8;
                int visual_row = 7 - rank;

                sprite.setPosition(sf::Vector2f(Settings::BOARD_OFFSET_X + file * Settings::TILE_SIZE, 
                                                Settings::BOARD_OFFSET_Y + visual_row * Settings::TILE_SIZE)
                );

                float scale = Settings::TILE_SIZE / sprite.getLocalBounds().size.x;
                sprite.setScale({ scale, scale });

                window_->draw(sprite);
            }
        }
        
        void draw_hints() {
            if (!selected_square_) return;

            Types::ui64 pseudo_moves = game_state_.get_possible_moves(*selected_square_);

            while (pseudo_moves) {
                int i = Bitwise::pop_lsb(pseudo_moves);
                
                int r = i / 8;
                int c = i % 8;
                int visual_row = 7 - r;

                Types::Move m = game_state_.create_move(*selected_square_, i);

                if (game_state_.is_move_legal(m)) {
                    sf::CircleShape hint;
                    if (m.is_capture()) {
                        hint.setRadius(Settings::TILE_SIZE * 0.40f);
                        hint.setOutlineThickness(4.f);
                        hint.setOutlineColor(sf::Color(0, 0, 0, 60));
                        hint.setFillColor(sf::Color::Transparent);
                    }
                    else {
                        hint.setRadius(Settings::TILE_SIZE * 0.12f);
                        hint.setFillColor(sf::Color(0, 0, 0, 60));
                    }

                    float x = Settings::BOARD_OFFSET_X + c * Settings::TILE_SIZE + (Settings::TILE_SIZE / 2.f);
                    float y = Settings::BOARD_OFFSET_Y + visual_row * Settings::TILE_SIZE + (Settings::TILE_SIZE / 2.f);

                    hint.setOrigin(hint.getGeometricCenter());
                    hint.setPosition({ x, y });

                    window_->draw(hint);
                }
            }
        }

        void draw_board_elements() {
            draw_board();
            draw_hints();
            draw_pieces();
        }

        void draw_game_buttons() {
            white_card_->draw(*window_);
            black_card_->draw(*window_);
            move_list_->draw(*window_);

            if (current_mode_ == Mode::AI) {
                window_->draw(*engine_label_);
                window_->draw(*engine_info_);

                for (int button_idx = 0; button_idx < think_time_buttons_.size(); ++button_idx) {
                    think_time_buttons_[button_idx]->draw(*window_);
                }
            }
			resign_button_->draw(*window_);
			undo_button_->draw(*window_);
        }

        void draw_menu_buttons() {
            float center_x = (Settings::WINDOW_WIDTH - 100.f) / 2.0f;
            float start_y = Settings::WINDOW_HEIGHT * 0.35f;
            float spacing = 60.0f;

            sf::Vector2f pos1 = { center_x, start_y };
            sf::Vector2f pos2 = { center_x, start_y + spacing };
            sf::Vector2f pos3 = { center_x, start_y + (spacing * 2) };

            if (current_screen_ == Screens::MainMenu || current_screen_ == Screens::GameEnd) {
                new_game_button_->set_position(pos1);
                new_game_button_->draw(*window_);

                mode_button_->set_position(pos2);
                mode_button_->draw(*window_);

                quit_button_->set_position(pos3);
                quit_button_->draw(*window_);
            }
            else if (current_screen_ == Screens::PauseMenu) {
                continue_button_->set_position(pos1);
                continue_button_->draw(*window_);

                quit_button_->set_position(pos2);
                quit_button_->draw(*window_);
            }
        }

        void draw_overlay(const std::string& title_text = "Chess") {
            sf::RectangleShape dimmer({ (float)Settings::WINDOW_WIDTH, (float)Settings::WINDOW_HEIGHT });
            dimmer.setFillColor(sf::Color(0, 0, 0, 180));
            window_->draw(dimmer);

            sf::Text title(font_, title_text, 50);
            title.setFillColor(sf::Color::White);
            sf::FloatRect b = title.getLocalBounds();
            title.setOrigin(b.getCenter());
            title.setPosition({ Settings::WINDOW_WIDTH / 2.0f, Settings::WINDOW_HEIGHT * 0.2f });
            window_->draw(title);
        }

        void draw_menu() {
			draw_board_elements();
            draw_overlay();
            draw_menu_buttons();
        }

        void draw_game() {
			draw_board_elements();
            draw_game_buttons();
        }
	};
}
