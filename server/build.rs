fn main() {
  let config = slint_build::CompilerConfiguration::new().with_library_paths(
    std::collections::HashMap::from([(
      "material".to_string(),
      ("ui/material/material.slint").into(),
    )]),
  );

  slint_build::compile_with_config("ui/main.slint", config).unwrap();
}
