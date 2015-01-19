class ExampleApp < Waah::App

  def setup
    rate 30.0
  end

  def draw
    clear

    font "Sans", :bold
    font_size 160
    text 200, 150, "ENTER   |   ME"

    if pointer.in?
      color 0xff, 0, 0
    else
      color 0xff, 0xff, 0xff
    end
    fill

  end
end

app = ExampleApp.new 500, 500, "In Example"
app.run
