class ExampleApp < Waah::App

  def setup
    rate 1.0

    if android?
      @image = Waah::Image.asset 'images/sky.jpg'
    else
      @image = Waah::Image.load 'examples/images/sky.jpg'
    end
  end

  def draw
    clear

    font_size 28
    translate 100, 100 do
      rotate Math::PI / 2.0 do
        text 0, 0, "THE SKY IS THE LIMIT"
      end
    end

    image @image
    fill

    5.times do |index|
      circle 40, (index + 1) * 50, 20
      image @image
      fill
    end

  end

end

app = ExampleApp.new 500, 500, "Image Example"
app.run
