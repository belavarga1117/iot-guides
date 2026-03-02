/**
 * mesh-topology.js — D3.js Thread Mesh Topology Visualizer
 *
 * Force-directed graph showing:
 * - Mesh nodes (routers, end devices, border router, leader)
 * - Links between nodes with quality indicators
 * - Self-healing: nodes appearing/disappearing with animation
 *
 * THE killer feature — when you unplug a node, see it vanish and
 * links reroute in real-time.
 */

const MeshTopology = (() => {
  let svg, simulation;
  let width = 280;
  let height = 300;
  let currentNodes = [];
  let currentLinks = [];

  function init() {
    const container = document.getElementById("mesh-container");
    width = container.clientWidth || 280;
    height = container.clientHeight || 300;

    svg = d3.select("#mesh-svg")
      .attr("viewBox", `0 0 ${width} ${height}`);

    // Force simulation
    simulation = d3.forceSimulation()
      .force("link", d3.forceLink().id(d => d.rloc16).distance(60))
      .force("charge", d3.forceManyBody().strength(-100))
      .force("center", d3.forceCenter(width / 2, height / 2))
      .force("collision", d3.forceCollide().radius(20))
      .on("tick", ticked);

    // Legend
    const legend = svg.append("g")
      .attr("transform", `translate(10, ${height - 60})`);

    const legendItems = [
      { label: "Border Router", color: "#44ff88", r: 7 },
      { label: "Leader", color: "#ffaa00", r: 6 },
      { label: "Router", color: "#00d4ff", r: 5 },
      { label: "End Device", color: "#888", r: 4 },
    ];

    legendItems.forEach((item, i) => {
      const g = legend.append("g")
        .attr("transform", `translate(0, ${i * 14})`);
      g.append("circle")
        .attr("r", item.r)
        .attr("cx", 6)
        .attr("cy", 0)
        .attr("fill", item.color);
      g.append("text")
        .attr("x", 18)
        .attr("y", 4)
        .attr("fill", "#666")
        .attr("font-size", "9px")
        .text(item.label);
    });
  }

  function update(meshData) {
    if (!meshData || !meshData.nodes) return;

    const nodes = meshData.nodes.map(n => {
      // Preserve position if node existed before
      const existing = currentNodes.find(cn => cn.rloc16 === n.rloc16);
      return {
        ...n,
        x: existing ? existing.x : width / 2 + (Math.random() - 0.5) * 50,
        y: existing ? existing.y : height / 2 + (Math.random() - 0.5) * 50,
      };
    });

    const links = meshData.links.map(l => ({
      source: l.source,
      target: l.target,
      link_quality: l.link_quality,
    }));

    // Detect nodes that left (for self-healing animation)
    const newRlocs = new Set(nodes.map(n => n.rloc16));
    const removedNodes = currentNodes.filter(n => !newRlocs.has(n.rloc16));

    // Animate removed nodes
    if (removedNodes.length > 0) {
      svg.selectAll(".mesh-node")
        .filter(d => removedNodes.some(r => r.rloc16 === d.rloc16))
        .transition()
        .duration(500)
        .attr("opacity", 0)
        .remove();
    }

    currentNodes = nodes;
    currentLinks = links;

    // --- Links ---
    const linkSelection = svg.selectAll(".mesh-link")
      .data(links, d => `${d.source}-${d.target}`);

    linkSelection.exit()
      .transition()
      .duration(300)
      .attr("stroke-opacity", 0)
      .remove();

    const linkEnter = linkSelection.enter()
      .append("line")
      .attr("class", d => `mesh-link quality-${d.link_quality}`)
      .attr("stroke-opacity", 0);

    linkEnter.transition()
      .duration(500)
      .attr("stroke-opacity", d => 0.2 + d.link_quality * 0.2);

    const allLinks = linkEnter.merge(linkSelection);

    // --- Nodes ---
    const nodeSelection = svg.selectAll(".mesh-node")
      .data(nodes, d => d.rloc16);

    nodeSelection.exit()
      .transition()
      .duration(500)
      .attr("opacity", 0)
      .remove();

    const nodeEnter = nodeSelection.enter()
      .append("g")
      .attr("class", d => `mesh-node ${d.role}`)
      .attr("opacity", 0)
      .call(d3.drag()
        .on("start", dragStarted)
        .on("drag", dragged)
        .on("end", dragEnded));

    nodeEnter.append("circle")
      .attr("r", d => nodeRadius(d.role))
      .attr("fill", d => nodeColor(d.role))
      .attr("stroke", d => nodeColor(d.role))
      .attr("stroke-width", 2)
      .attr("stroke-opacity", 0.3);

    nodeEnter.append("text")
      .attr("class", "mesh-label")
      .attr("y", d => nodeRadius(d.role) + 12)
      .text(d => d.node_id || d.rloc16);

    nodeEnter.transition()
      .duration(500)
      .attr("opacity", 1);

    const allNodes = nodeEnter.merge(nodeSelection);

    // Update simulation
    simulation.nodes(nodes);
    simulation.force("link").links(links);
    simulation.alpha(0.3).restart();

    // Store references for ticked()
    svg._allLinks = allLinks;
    svg._allNodes = allNodes;
  }

  function ticked() {
    const allLinks = svg._allLinks;
    const allNodes = svg._allNodes;
    if (!allLinks || !allNodes) return;

    allLinks
      .attr("x1", d => d.source.x)
      .attr("y1", d => d.source.y)
      .attr("x2", d => d.target.x)
      .attr("y2", d => d.target.y);

    allNodes
      .attr("transform", d => `translate(${d.x}, ${d.y})`);
  }

  function nodeColor(role) {
    switch (role) {
      case "border-router": return "#44ff88";
      case "leader": return "#ffaa00";
      case "router": return "#00d4ff";
      default: return "#888";
    }
  }

  function nodeRadius(role) {
    switch (role) {
      case "border-router": return 10;
      case "leader": return 9;
      case "router": return 7;
      default: return 5;
    }
  }

  function dragStarted(event, d) {
    if (!event.active) simulation.alphaTarget(0.1).restart();
    d.fx = d.x;
    d.fy = d.y;
  }

  function dragged(event, d) {
    d.fx = event.x;
    d.fy = event.y;
  }

  function dragEnded(event, d) {
    if (!event.active) simulation.alphaTarget(0);
    d.fx = null;
    d.fy = null;
  }

  return { init, update };
})();
