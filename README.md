# Oregon State University Course Tree
An interactive graph of all OSU courses that lets you visually the connections between each.

## System Plan
```mermaid
flowchart TD
    A[User]
    B[Course Data]

    subgraph GUI
        A --> D[Force-Directed Network Graph]
        A --> E[Information Window]
        A --> F[Fizzy Search Feature]
        A --> G[Physics Settings]
    end

    subgraph Rendering["Rendering / Physics"]
        D --> H[Node Shader]
        D --> I[Edge Shader]
        D --> J[Physics Shader]
        D --> K[Selection Shader]
    end

    subgraph Buffers["GPU Buffers"]
        B --> L[Node Buffer]
        B --> M[Edge Buffer]
        N[Selection Buffer]
    end

    L --> H
    L --> I
    L --> J
    L --> K

    M --> H
    M --> I
    M --> J
    M --> K

    N --> K
    
    G --> J
```

# Notes
The code to generate the JSON course data is not a part of this repository and is kept private.