module;
#include <memory>
#include <SFML/Graphics.hpp>
#include <iostream>

export module WindowManager;
import Types;
import Position;
import Button;
import GameState;
import Bitboards;
import EngineInterface;

namespace Settings {
	constexpr int WINDOW_WIDTH = 800;
	constexpr int WINDOW_HEIGHT = 600;
	constexpr int BOARD_SIZE = 8;

    constexpr int TILE_SIZE = (WINDOW_HEIGHT * 0.9) / BOARD_SIZE;

    constexpr int BOARD_OFFSET_X = 50;
    constexpr int BOARD_OFFSET_Y = (WINDOW_HEIGHT - (BOARD_SIZE * TILE_SIZE)) / 2;

    constexpr int UI_X = BOARD_OFFSET_X + (BOARD_SIZE * TILE_SIZE) + 40;

    // Tiles
    constexpr sf::Color WHITE_TILE_COLOR(240, 217, 181);
    constexpr sf::Color BLACK_TILE_COLOR(181, 136, 99);
	constexpr sf::Color HIGHLIGHT_COLOR(170, 160, 0, 150);

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

    export class GameWindow {
		GameState::GameState game_state_;
		std::unique_ptr<sf::RenderWindow> window_ = nullptr;
		Screens current_screen_ = Screens::Game;
		std::array<std::array<sf::Texture,2>,6> pieces_texture_;
        sf::Font font_;

        std::unique_ptr<Button::TextButton> undo_button_;
        std::unique_ptr<Button::TextButton> resign_button_;

        std::unique_ptr<Button::TextButton> continue_button_;
        std::unique_ptr<Button::TextButton> quit_button_;
        std::unique_ptr<Button::TextButton> new_game_button_;

		std::array<std::array<std::unique_ptr<Button::ImageButton>, 2>,4> promotion_buttons_;

		std::optional<int> selected_square_ = std::nullopt;
		std::optional<int> clicked_square_ = std::nullopt;

		//EngineInterface::EngineBridge engine_;
	public:
        GameWindow() {
            window_ = std::make_unique<sf::RenderWindow>(sf::VideoMode({ Settings::WINDOW_WIDTH, Settings::WINDOW_HEIGHT }), "Chess");
            window_->setFramerateLimit(60);

            load_resources();

            undo_button_ = std::make_unique<Button::TextButton>(font_, "Undo", sf::Vector2f(140.f, 40.f));
            resign_button_ = std::make_unique<Button::TextButton>(font_, "Resign", sf::Vector2f(140.f, 40.f));
			continue_button_ = std::make_unique<Button::TextButton>(font_, "Continue", sf::Vector2f(140.f, 40.f));
			quit_button_ = std::make_unique<Button::TextButton>(font_, "Quit", sf::Vector2f(140.f, 40.f));
			new_game_button_ = std::make_unique<Button::TextButton>(font_, "New Game", sf::Vector2f(140.f, 40.f));

            int padding = 20;
            const char piece_chars[] = { 'p', 'r', 'n', 'b', 'q', 'k' };
			int start_x = (Settings::WINDOW_WIDTH - (4 * Settings::TILE_SIZE + 3 * padding)) / 2;
			int start_y = (Settings::WINDOW_HEIGHT - Settings::TILE_SIZE) / 2;

            for (int color = 0; color < 2; ++color) {
                int start_x = (Settings::WINDOW_WIDTH - (4 * Settings::TILE_SIZE + 3 * padding)) / 2;
                for(int type = 0; type < 4; ++type) {          
                    promotion_buttons_[type][color] = std::make_unique<Button::ImageButton>(pieces_texture_[type + 1][color], sf::Vector2f(Settings::TILE_SIZE, Settings::TILE_SIZE));
                    promotion_buttons_[type][color]->set_on_click([this, type, color, piece_chars]() {
                        char choice = piece_chars[type + 1];
                      
                        GameState::GameResult res = game_state_.try_move(*selected_square_, *clicked_square_, choice);
						current_screen_ = res != GameState::GameResult::Ongoing ? Screens::GameEnd : Screens::Game;

                        selected_square_ = std::nullopt;
                        clicked_square_ = std::nullopt; }
                    );
					promotion_buttons_[type][color]->set_position(sf::Vector2f( start_x, start_y ));
                    start_x += Settings::TILE_SIZE + padding;
				}
            }

            undo_button_ ->set_on_click([this]() {
                game_state_.undo_move();
                std::cout << "Undo clicked\n";
			});
            resign_button_->set_on_click([this]() {
				current_screen_ = Screens::MainMenu;
				std::cout << "Resign clicked\n";
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
				std::cout << "New Game clicked\n";
			});

            float ui_x = static_cast<float>(Settings::BOARD_OFFSET_X + (Settings::BOARD_SIZE * Settings::TILE_SIZE) + 30);
            undo_button_->set_position({ ui_x, Settings::WINDOW_HEIGHT * 0.75f });
            resign_button_->set_position({ ui_x, Settings::WINDOW_HEIGHT * 0.85f });
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
                }
            }

            if (!font_.openFromFile("assets/Fonts/CooperHewitt.otf"))
                std::cerr << "Failed to load font" << '\n';
		}

        void handle_events() {
            while (const std::optional event = window_->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window_->close();
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
            }

            if (auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    resign_button_->check_click(mouse_pos);
                    undo_button_->check_click(mouse_pos);

                    auto clicked_sq = get_square_from_mouse(mouse_pos);
                    if (!clicked_sq) return;

                    if (!selected_square_) {
                        const auto piece = game_state_.get_mailbox()[*clicked_sq];
                        if (!piece.is_none() && (piece.is_white() == game_state_.white_to_move())) {
                            selected_square_ = clicked_sq;
                        }
                    }
                    else {
                        if (clicked_sq == selected_square_) {
                            selected_square_ = std::nullopt;
                        }
                        else {
                            clicked_square_ = clicked_sq;
                            Types::Move m = game_state_.create_move(*selected_square_, *clicked_square_);
                            if (m.is_promo()) {
                                current_screen_ = Screens::Promotion;
                            }
                            else {
                                // 2. Standard execution
                                GameState::GameResult res = game_state_.try_move(*selected_square_, *clicked_square_);
                                if(res != GameState::GameResult::Ongoing){
                                    current_screen_ = Screens::GameEnd;
								}
                                selected_square_ = std::nullopt;
                                clicked_square_ = std::nullopt;
                            }
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

        int get_eval() {
            return 0;
        }

        float get_win_probability(int cp) {
            return 1.0f / (1.0f + std::pow(10.0f, -static_cast<float>(cp) / 400.0f));
        }

        void draw_board() {
            using Settings::TILE_SIZE;
            sf::RectangleShape white_square({ TILE_SIZE, TILE_SIZE });
            sf::RectangleShape black_square({ TILE_SIZE, TILE_SIZE });
			sf::RectangleShape selection_highlight({ TILE_SIZE, TILE_SIZE });

            white_square.setFillColor(Settings::WHITE_TILE_COLOR);
            black_square.setFillColor(Settings::BLACK_TILE_COLOR);
            selection_highlight.setFillColor(Settings::HIGHLIGHT_COLOR);

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
			}

			int cp_score = get_eval();
            float win_prob = get_win_probability(cp_score);
            float total_board_height = Settings::BOARD_SIZE * Settings::TILE_SIZE;

            sf::RectangleShape black_bar({ 20.f, total_board_height });
            black_bar.setFillColor(Settings::EVAL_BAR_BLACK);
            black_bar.setPosition(sf::Vector2f(5, Settings::BOARD_OFFSET_Y));

            float white_height = total_board_height * win_prob;
            sf::RectangleShape white_bar({ 20.f, white_height });
            white_bar.setFillColor(Settings::EVAL_BAR_WHITE);

            white_bar.setPosition(sf::Vector2f(5, Settings::BOARD_OFFSET_Y + total_board_height - white_height));

            window_->draw(black_bar);
            window_->draw(white_bar);
            draw_coordinates();
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
			resign_button_->draw(*window_);
			undo_button_->draw(*window_);
        }

        void draw_menu_buttons() {
            float center_x = (Settings::WINDOW_WIDTH + 70.2f) / 2.0f;
            float start_y = Settings::WINDOW_HEIGHT* 0.4f;
            float spacing = 70.0f;
            sf::Vector2f top_pos = { center_x - 100, start_y };
            sf::Vector2f bottom_pos = { center_x - 100, start_y + spacing };

            if (current_screen_ == Screens::MainMenu || current_screen_ == Screens::GameEnd) {
                new_game_button_->set_position(top_pos);
                new_game_button_->draw(*window_);

                quit_button_->set_position(bottom_pos);
                quit_button_->draw(*window_);
            }
            else if (current_screen_ == Screens::PauseMenu) {
                continue_button_->set_position(top_pos);
                continue_button_->draw(*window_);

                quit_button_->set_position(bottom_pos);
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
