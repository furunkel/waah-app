class ExampleApp < Waah::App

  def setup
    rate 30.0
  end

  def draw
    clear

    if pointer.down?
      circle pointer.x, pointer.y, 10
      color 0xff, 0xff, 0xff
      line_width 2
      stroke
    end
  end
end

app = ExampleApp.new 500, 500, "Pointer Example"
app.run
