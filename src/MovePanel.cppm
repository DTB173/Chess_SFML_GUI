module;
#include <SFML/Graphics.hpp>

export module MovePanel;
import Types;
import MoveGen;

namespace Moves {
    export class MoveListUI {
        const std::vector<Types::Move>& move_stack_;
        sf::Font& font_;
        sf::RectangleShape background_;
        float line_spacing_ = 28.f;

    public:
        MoveListUI(const std::vector<Types::Move>& moves, sf::Font& font, sf::Vector2f size)
            : move_stack_(moves), font_(font)
        {
            background_.setSize(size);
            background_.setFillColor(sf::Color(45, 43, 40, 180));
        }

        void set_position(float x, float y) {
            background_.setPosition({ x, y });
        }

        void draw(sf::RenderTarget& target) {
            target.draw(background_);

            // Calculate how many lines fit
            int max_lines = static_cast<int>(background_.getSize().y / line_spacing_) - 1;
            int total_turns = static_cast<int>((move_stack_.size() + 1) / 2);

            // Sliding window logic: always show the latest moves
            int start_turn = std::max(0, total_turns - max_lines);

            sf::Text text(font_, "", 18);
            for (int i = start_turn; i < total_turns; ++i) {
                float y_pos = background_.getPosition().y + 10 + (i - start_turn) * line_spacing_;

                // 1. Draw Move Number
                text.setString(std::to_string(i + 1) + ".");
                text.setFillColor(sf::Color(120, 120, 120));
                text.setPosition({ background_.getPosition().x + 10, y_pos });
                target.draw(text);

                // 2. Draw White Move
                text.setString(Types::move_to_string(move_stack_[i * 2]));
                text.setFillColor(sf::Color::White);
                text.setPosition({ background_.getPosition().x + 45, y_pos });
                target.draw(text);

                // 3. Draw Black Move (if exists)
                if (i * 2 + 1 < move_stack_.size()) {
                    text.setString(Types::move_to_string(move_stack_[i * 2 + 1]));
                    text.setPosition({ background_.getPosition().x + 105, y_pos });
                    target.draw(text);
                }
            }
        }
    };
}