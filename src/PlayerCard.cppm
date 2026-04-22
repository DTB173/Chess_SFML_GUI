module;
#include <SFML/Graphics.hpp>
#include <memory>
export module PlayerCard;

namespace Cards {
	export class PlayerCard {
		sf::RectangleShape bg_;
		sf::Text text_;
		std::array<sf::Color,2> colors_;
	public:
		PlayerCard(float width, float height, sf::Color color_idle, sf::Color color_active, sf::Font& font, const std::string& text) 
			:text_(font, text, 22)
		{
			bg_ = sf::RectangleShape(sf::Vector2f(width, height));
			bg_.setFillColor(color_idle);
			colors_ = { color_idle, color_active };
		}

		void set_position(float x, float y) {
			bg_.setPosition({ x,y });
			center_text();
		}

		void set_text(const std::string& text) {
			text_.setString(text);
			center_text();
		}

		void draw(sf::RenderTarget& target) const {
			target.draw(bg_);
			target.draw(text_);
		}

		void set_active() {
			bg_.setFillColor(colors_[1]);
			bg_.setOutlineThickness(2.f);
			bg_.setOutlineColor(sf::Color::White);
		}

		void set_inactive() {
			bg_.setFillColor(colors_[0]);
			bg_.setOutlineThickness(0.f);
		}

		auto get_size()const {
			return bg_.getSize();
		}
	private:
		void center_text() {
			auto text_bounds = text_.getLocalBounds();

			text_.setOrigin({
				text_bounds.position.x + text_bounds.size.x / 2.f,
				text_bounds.position.y + text_bounds.size.y / 2.f
				});

			auto bg_size = bg_.getSize();
			auto bg_pos = bg_.getPosition();

			text_.setPosition({
				bg_pos.x + bg_size.x / 2.f,
				bg_pos.y + bg_size.y / 2.f
				});
		}
	};
}