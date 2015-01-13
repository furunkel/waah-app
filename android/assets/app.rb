class ExampleApp < Waah::App
  def setup
    rate 40.0
    log :verbose, "Running app" if android?
  end

  def draw
    clear

    @r ||= 0
    translate 100, 100 do
      rotate((@r % 100) / 100 * 2 * 3.14) do
        rounded_rect 0, 0, 50, 50, 3
      end
    end
    @r += 5

    if pointer.pressed? && pointer.in?
      color 0, 0xff, 0
      keyboard.toggle
    else
      color 0xff, 0, 0
    end
    fill true
    color 0, 0xff, 0
    stroke

    redraw
  end
end

app = ExampleApp.new 100, 100

begin
  app.run
rescue Exception => e
  app.log :error, e.inspect
end
