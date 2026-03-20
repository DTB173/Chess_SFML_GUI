#include <SFML/Graphics.hpp>
#include <functional>

export module Button;

namespace Button {

    export class BaseButton {
    protected:
        sf::Vector2f position_;
        std::function<void()> on_click_;
        bool is_hovered_ = false;

    public:
        virtual ~BaseButton() = default;

        void set_position(const sf::Vector2f& pos) {
            position_ = pos;
            on_position_changed();
        }

        void set_on_click(std::function<void()> callback) {
            on_click_ = std::move(callback);
        }

        virtual void update_hover(const sf::Vector2f& mouse_pos) = 0;

        bool check_click(const sf::Vector2f& mouse_pos) {
            if (on_click_ && contains(mouse_pos)) {
                on_click_();
                return true;
            }
            return false;
        }

        virtual void draw(sf::RenderTarget& target) const = 0;

    protected:
        virtual bool contains(const sf::Vector2f& point) const = 0;
        virtual void on_position_changed() {}
    };


    export class TextButton final : public BaseButton {
    private:
        sf::RectangleShape shape_;
        sf::Text           text_;

        sf::Color idle_color_ = sf::Color(70, 70, 70);
        sf::Color hover_color_ = sf::Color(100, 100, 100);

    public:
        TextButton(const sf::Font& font,
            const std::string& label,
            sf::Vector2f size = { 200.f, 50.f },
            unsigned int char_size = 20u) :text_(font, label, char_size)
        {
            shape_.setSize(size);
            shape_.setFillColor(idle_color_);
            shape_.setOutlineColor(sf::Color(150, 150, 150));
            shape_.setOutlineThickness(2.f);

            text_.setFont(font);
            text_.setString(label);
            text_.setCharacterSize(char_size);
            text_.setFillColor(sf::Color::White);

            on_position_changed();
        }

        void update_hover(const sf::Vector2f& mouse_pos) override {
            bool hovered = shape_.getGlobalBounds().contains(mouse_pos);
            if (hovered != is_hovered_) {
                is_hovered_ = hovered;
                shape_.setFillColor(hovered ? hover_color_ : idle_color_);
            }
        }

        void draw(sf::RenderTarget& target) const override {
            target.draw(shape_);
            target.draw(text_);
        }

        void set_idle_color(sf::Color c) { idle_color_ = c;  if (!is_hovered_) shape_.setFillColor(c); }
        void set_hover_color(sf::Color c) { hover_color_ = c; }

    protected:
        bool contains(const sf::Vector2f& point) const override {
            return shape_.getGlobalBounds().contains(point);
        }

        void on_position_changed() override {
            shape_.setPosition(position_);

            sf::FloatRect tb = text_.getLocalBounds();
            sf::Vector2f center = position_ + shape_.getSize() * 0.5f;
            text_.setOrigin(tb.getCenter());
            text_.setPosition(center);
        }
    };


    export class ImageButton final : public BaseButton {
    private:
        sf::Sprite         sprite_;
        sf::RectangleShape background_;

        sf::Color bg_idle_color_ = sf::Color(50, 50, 50);
        sf::Color bg_hover_color_ = sf::Color(80, 80, 90);

        float padding_ = 4.f;

    public:
        ImageButton(const sf::Texture& texture,
            sf::Vector2f desired_size = { 0.f, 0.f }):sprite_(texture)
        {
            sprite_.setTexture(texture);

            if (desired_size != sf::Vector2f(0.f, 0.f)) {
                sf::FloatRect bounds = sprite_.getLocalBounds();
                sf::Vector2f scale(
                    desired_size.x / bounds.size.x,
                    desired_size.y / bounds.size.y
                );
                sprite_.setScale(scale);
            }

            update_background_size();

            background_.setFillColor(bg_idle_color_);
            background_.setOutlineColor(sf::Color(120, 120, 120));
            background_.setOutlineThickness(1.f);

            on_position_changed();
        }

        void update_hover(const sf::Vector2f& mouse_pos) override {
            bool hovered = contains(mouse_pos);
            if (hovered != is_hovered_) {
                is_hovered_ = hovered;
                background_.setFillColor(hovered ? bg_hover_color_ : bg_idle_color_);
            }
        }

        void draw(sf::RenderTarget& target) const override {
            target.draw(background_);
            target.draw(sprite_);
        }

        void set_background_idle_color(sf::Color c) {
            bg_idle_color_ = c;
            if (!is_hovered_) background_.setFillColor(c);
        }

        void set_background_hover_color(sf::Color c) {
            bg_hover_color_ = c;
        }

        void set_padding(float padding) {
            padding_ = padding;
            update_background_size();
        }

    protected:
        bool contains(const sf::Vector2f& point) const override {
            return background_.getGlobalBounds().contains(point);
        }

        void on_position_changed() override {
            sf::Vector2f bg_pos = position_;

            background_.setPosition(bg_pos);

            sf::Vector2f sprite_offset(
                padding_,
                padding_
            );

            sprite_.setPosition(bg_pos + sprite_offset);
        }

    private:
        void update_background_size() {
            sf::FloatRect sprite_bounds = sprite_.getGlobalBounds(); 

            background_.setSize(sf::Vector2f(
                sprite_bounds.size.x + 2 * padding_,
                sprite_bounds.size.y + 2 * padding_
            ));
        }
    };

}