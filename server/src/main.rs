use slint::ComponentHandle;

slint::include_modules!();

fn main() {
  HelloWorld::new().unwrap().run().unwrap();
}
